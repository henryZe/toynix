#include <lib.h>
#include <fs/fs.h>

#define BLKNO2ADDR(x) (void *)(DISKMAP + x * BLKSIZE)

// --------------------------------------------------------------
// Super block
// --------------------------------------------------------------

// Validate the file system super-block.
static void
check_super(void)
{
	if (super->s_magic != FS_MAGIC)
		panic("bad file system magic number");

	if (super->s_nblocks > DISKSIZE / BLKSIZE)
		panic("file system is too large");

	cprintf("superblock is good\n");
}

// --------------------------------------------------------------
// Free block bitmap
// --------------------------------------------------------------

// Check to see if the block bitmap indicates that block 'blockno' is free.
// Return 1 if the block is free, 0 if not.
bool
block_is_free(uint32_t blockno)
{
	if (super == NULL || blockno >= super->s_nblocks)
		return 0;

	if (bitmap[blockno / 32] & (1 << (blockno % 32)))
		return 1;

	return 0;
}

// Mark a block free in the bitmap
static void
free_block(uint32_t blockno)
{
	// Blockno zero is the null pointer of block numbers.
	if (blockno == 0)
		panic("attempt to free zero block");

	bitmap[blockno / 32] |= 1 << (blockno % 32);
	flush_block(&bitmap[blockno / 32]);
}

// Search the bitmap for a free block and allocate it.  When you
// allocate a block, immediately flush the changed bitmap block
// to disk.
//
// Return block number allocated on success,
// -E_NO_DISK if we are out of blocks.
int
alloc_block(void)
{
	int blockno;

	// The bitmap consists of one or more blocks.  A single bitmap block
	// contains the in-use bits for BLKBITSIZE blocks.  There are
	// super->s_nblocks blocks in the disk altogether.
	for (blockno = 0; blockno < super->s_nblocks; blockno++) {
		if (bitmap[blockno / 32] & (1 << (blockno % 32)))
			break;
	}

	if (blockno == super->s_nblocks)
		return -E_NO_DISK;

	/* clean free bit */
	bitmap[blockno / 32] &= ~(1 << (blockno % 32));
	/* flush the bitmap back to disk */
	flush_block(&bitmap[blockno / 32]);

	return blockno;
}

// Validate the file system bitmap.
//
// Check that all reserved blocks -- 0, 1, and the bitmap blocks themselves --
// are all marked as in-use.
static void
check_bitmap(void)
{
	uint32_t i;

	// Make sure all bitmap blocks are marked in-use
	for (i = 0; i * BLKBITSIZE < super->s_nblocks; i++)
		assert(!block_is_free(2 + i));

	// Make sure the reserved and root blocks are marked in-use.
	assert(!block_is_free(0));
	assert(!block_is_free(1));

	cprintf("bitmap is good\n");
}

// --------------------------------------------------------------
// File system structures
// --------------------------------------------------------------

// Initialize the file system
void
fs_init(void)
{
	static_assert(sizeof(struct File) == 256);

	// Find a toynix disk. Use the second IDE disk (number 1) if available.
	if (ide_probe_disk1())
		ide_set_disk(1);
	else
		ide_set_disk(0);

	bc_init();

	// Set "super" to point to the super block.
	super = diskaddr(1);
	check_super();

	// Set "bitmap" to the beginning of the first bitmap block.
	bitmap = diskaddr(2);
	check_bitmap();
}

// Find the disk block number slot for the 'filebno'th block in file 'f'.
// Set '*ppdiskbno' to point to that slot.
// The slot will be one of the f->f_direct[] entries,
// or an entry in the indirect block.
// When 'alloc' is set, this function will allocate an indirect block
// if necessary.
//
// Returns:
//	0 on success (but note that *ppdiskbno might equal 0).
//	-E_NOT_FOUND if the function needed to allocate an indirect block, but
//		alloc was 0.
//	-E_NO_DISK if there's no space on the disk for an indirect block.
//	-E_INVAL if filebno is out of range (it's >= NDIRECT + NINDIRECT).
static int
file_block_walk(struct File *f, uint32_t filebno, uint32_t **ppdiskbno, bool alloc)
{
	int ret, blockno;
	uint32_t *block_addr;

	if (!ppdiskbno)
		return -E_INVAL;

	if (filebno >= (NINDIRECT + NDIRECT))
		return -E_INVAL;

	if (filebno < NDIRECT) {
		*ppdiskbno = &f->f_direct[filebno];
		return 0;
	}

	if (!f->f_indirect) {
		if (!alloc)
			return -E_NOT_FOUND;

		blockno = alloc_block();
		if (blockno < 0)
			return -E_NO_DISK;

		f->f_indirect = blockno;
		block_addr = BLKNO2ADDR(blockno);
		ret = sys_page_alloc(0, block_addr, PTE_SYSCALL);
		if (ret < 0)
			panic("%s sys_page_alloc %e", __func__, ret);

		/* initialize f->f_indirect block */
		memset(block_addr, 0, BLKSIZE);

	} else {
		block_addr = BLKNO2ADDR(f->f_indirect);
	}

	*ppdiskbno = &block_addr[filebno - NDIRECT];
	return 0;
}

// skip over slashes
static const char *
skip_slash(const char *p)
{
	while (*p == '/')
		p++;

	return p;
}

// Set *blk to the address in memory where the filebno'th
// block of file 'f' would be mapped.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_NO_DISK if a block needed to be allocated but the disk is full.
//	-E_INVAL if filebno is out of range.
int
file_get_block(struct File *f, uint32_t filebno, char **blk)
{
	int ret;
	uint32_t *pdiskbno = NULL;

	ret = file_block_walk(f, filebno, &pdiskbno, 1);
	if (ret < 0)
		return ret;

	if (!*pdiskbno) {
		ret = alloc_block();
		if (ret < 0)
			return ret;

		/* set struct File f->f_direct or f->f_indirect */
		*pdiskbno = ret;

		ret = sys_page_alloc(0, BLKNO2ADDR(*pdiskbno), PTE_SYSCALL);
		if (ret < 0)
			panic("%s sys_page_alloc %e", __func__, ret);
	}

	*blk = BLKNO2ADDR(*pdiskbno);
	return 0;
}

// Try to find a file named "name" in dir.  If so, set *file to it.
//
// Returns 0 and sets *file on success, < 0 on error.  Errors are:
//	-E_NOT_FOUND if the file is not found
static int
dir_lookup(struct File *dir, const char *name, struct File **file)
{
	int ret;
	uint32_t i, j, nblock;
	struct File *fp;
	char *blk = NULL;

	// Search dir for name.
	// We maintain the invariant that the size of a directory-file
	// is always a multiple of the file system's block size.
	assert((dir->f_size % BLKSIZE) == 0);

	nblock = dir->f_size / BLKSIZE;
	for (i = 0; i < nblock; i++) {
		ret = file_get_block(dir, i, &blk);
		if (ret < 0)
			return ret;

		fp = (struct File *)blk;
		for (j = 0; j < BLKFILES; j++) {
			if (strcmp(fp[j].f_name, name) == 0) {
				*file = &fp[j];
				return 0;
			}
		}
	}
	return -E_NOT_FOUND;
}

// Set *file to point at a free File structure in dir.  The caller is
// responsible for filling in the File fields.
static int
dir_alloc_file(struct File *dir, struct File **file)
{
	int ret;
	uint32_t n_block, i, j;
	char *blk;
	struct File *f;

	/* dir->f_size is aligned to BLKSIZE */
	assert((dir->f_size % BLKSIZE) == 0);

	n_block = dir->f_size / BLKSIZE;
	for (i = 0; i < n_block; i++) {
		ret = file_get_block(dir, i, &blk);
		if (ret < 0)
			return ret;

		f = (struct File *)blk;
		for (j = 0; j < BLKFILES; j++) {
			if (f[j].f_name[0] == '\0') {
				*file = &f[j];
				return 0;
			}
		}
	}

	/* expand dir */
	dir->f_size += BLKSIZE;
	ret = file_get_block(dir, i, &blk);
	if (ret < 0)
		return ret;

	f = (struct File *)blk;
	*file = &f[0];
	return 0;
}

// Evaluate a path name, starting at the root.
// On success, set *pf to the file we found
// and set *pdir to the directory the file is in.
// If we cannot find the file but find the directory
// it should be in, set *pdir and copy the final path
// element into lastelem.
static int
walk_path(const char *path, struct File **pdir,
			struct File **pfile, char *lastelem)
{
	const char *p;
	char name[MAXNAMELEN];
	struct File *dir, *file;
	int ret;

	/* absolute path required */
	if (*path != '/')
		return -E_BAD_PATH;

	path = skip_slash(path);
	file = &super->s_root;
	dir = NULL;
	name[0] = 0;

	if (pdir)
		*pdir = NULL;
	*pfile = NULL;

	while (*path != '\0') {
		dir = file;
		p = path;

		while (*path != '/' && *path != '\0')
			path++;

		if (path - p >= MAXNAMELEN)
			return -E_BAD_PATH;

		memmove(name, p, path - p);
		name[path - p] = '\0';
		path = skip_slash(path);

		if (dir->f_type != FTYPE_DIR)
			return -E_NOT_FOUND;

		ret = dir_lookup(dir, name, &file);
		if (ret < 0) {
			if (ret == -E_NOT_FOUND && *path == '\0') {
				if (pdir)
					*pdir = dir;
				if (lastelem)
					strcpy(lastelem, name);
				*pfile = NULL;
			}
			return ret;
		}
	}

	if (pdir)
		*pdir = dir;
	*pfile = file;
	return 0;
}

// --------------------------------------------------------------
// File operations
// --------------------------------------------------------------

// Create "path".  On success set *pf to point at the file and return 0.
// On error return < 0.
int
file_create(const char *path, struct File **pf, bool is_dir)
{
	char name[MAXPATHLEN];
	int ret;
	struct File *dir, *f;

	ret = walk_path(path, &dir, &f, name);
	if (!ret)
		return -E_FILE_EXISTS;

	if (ret != -E_NOT_FOUND || dir == NULL)
		return ret;

	ret = dir_alloc_file(dir, &f);
	if (ret < 0)
		return ret;

	strcpy(f->f_name, name);
	f->f_type = (is_dir ? FTYPE_DIR : FTYPE_REG);
	*pf = f;
	file_flush(dir);

	return 0;
}

// Open "path".  On success set *pf to point at the file and return 0.
// On error return < 0.
int
file_open(const char *path, struct File **pfile)
{
	return walk_path(path, NULL, pfile, NULL);
}

// Read count bytes from f into buf, starting from seek position
// offset.  This meant to mimic the standard pread function.
// Returns the number of bytes read, < 0 on error.
ssize_t
file_read(struct File *f, void *buf, size_t count, off_t offset)
{
	int ret, bn;
	off_t pos;
	char *blk;

	if (offset >= f->f_size)
		return 0;

	count = MIN(count, f->f_size - offset);

	for (pos = offset; pos < offset + count; ) {
		ret = file_get_block(f, pos / BLKSIZE, &blk);
		if (ret < 0)
			return ret;

		bn = MIN(BLKSIZE - pos % BLKSIZE, offset + count - pos);
		memmove(buf, blk + pos % BLKSIZE, bn);

		pos += bn;
		buf += bn;
	}

	return count;
}

// Write count bytes from buf into f, starting at seek position
// offset.  This is meant to mimic the standard pwrite function.
// Extends the file if necessary.
// Returns the number of bytes written, < 0 on error.
int
file_write(struct File *f, const void *buf, size_t count, off_t offset)
{
	int ret, bn;
	off_t pos;
	char *blk;

	/* Extend file if necessary */
	if (offset + count > f->f_size) {
		ret = file_set_size(f, offset + count);
		if (ret < 0)
			return ret;
	}

	for (pos = offset; pos < offset + count; ) {
		ret = file_get_block(f, pos / BLKSIZE, &blk);
		if (ret < 0)
			return ret;

		bn = MIN(BLKSIZE - pos % BLKSIZE, offset + count - pos);
		memmove(blk + pos % BLKSIZE, buf, bn);

		pos += bn;
		buf += bn;
	}

	return count;
}

// Flush the contents and metadata of file f out to disk.
// Loop over all the blocks in file.
// Translate the file block number into a disk block number
// and then check whether that disk block is dirty.  If so, write it out.
void
file_flush(struct File *f)
{
	int i;
	uint32_t *pdiskbno;

	for (i = 0; i < (f->f_size + BLKSIZE - 1) / BLKSIZE; i++) {
		if (file_block_walk(f, i, &pdiskbno, 0) < 0 ||
			pdiskbno == NULL || *pdiskbno == 0)
			continue;

		flush_block(diskaddr(*pdiskbno));
	}

	flush_block(f);
	if (f->f_indirect)
		flush_block(diskaddr(f->f_indirect));
}

// Close file
// Loop over all the blocks in file, and free page cache for recycling.
void file_close(struct File *f)
{
	int i;
	uint32_t *blockno;

	for (i = 0; i < NDIRECT; i++)
		sys_page_unmap(0, BLKNO2ADDR(f->f_direct[i]));

	if (f->f_indirect) {

		for (i = 0; i < (f->f_size / BLKSIZE - NDIRECT); i++) {

			blockno = BLKNO2ADDR(f->f_indirect);
			sys_page_unmap(0, BLKNO2ADDR(blockno[i]));
		}

		sys_page_unmap(0, BLKNO2ADDR(f->f_indirect));
	}

	sys_page_unmap(0, f);
}

// Remove a block from file f.  If it's not there, just silently succeed.
// Returns 0 on success, < 0 on error.
static int
file_free_block(struct File *f, uint32_t filebno)
{
	int ret;
	uint32_t *ptr;

	ret = file_block_walk(f, filebno, &ptr, 0);
	if (ret < 0)
		return ret;

	if (*ptr) {
		free_block(*ptr);
		sys_page_unmap(0, BLKNO2ADDR(*ptr));
		*ptr = 0;
	}

	return 0;
}

// Remove any blocks currently used by file 'f',
// but not necessary for a file of size 'newsize'.
// For both the old and new sizes, figure out the number of blocks required,
// and then clear the blocks from new_nblocks to old_nblocks.
// If the new_nblocks is no more than NDIRECT, and the indirect block has
// been allocated (f->f_indirect != 0), then free the indirect block too.
// (Remember to clear the f->f_indirect pointer so you'll know
// whether it's valid!)
// Do not change f->f_size.
static void
file_truncate_blocks(struct File *f, off_t newsize)
{
	int ret;
	uint32_t bno, old_nblocks, new_nblocks;

	old_nblocks = (f->f_size + BLKSIZE - 1) / BLKSIZE;
	new_nblocks = (newsize + BLKSIZE - 1) / BLKSIZE;

	for (bno = new_nblocks; bno < old_nblocks; bno++) {
		ret = file_free_block(f, bno);
		if (ret < 0)
			warn("file_free_block: %e", ret);
	}

	if (new_nblocks <= NDIRECT && f->f_indirect) {
		free_block(f->f_indirect);
		sys_page_unmap(0, BLKNO2ADDR(f->f_indirect));
		f->f_indirect = 0;
	}
}

// Set the size of file f, truncating or extending as necessary.
int
file_set_size(struct File *f, off_t newsize)
{
	if (f->f_size > newsize)
		file_truncate_blocks(f, newsize);

	f->f_size = newsize;
	/* update file meta data */
	flush_block(f);
	return 0;
}

// Sync the entire file system.  A big hammer.
void
fs_sync(void)
{
	int i;

	for (i = 1; i < super->s_nblocks; i++)
		flush_block(diskaddr(i));
}

int
file_remove(struct File *f)
{
	// delete file content
	file_truncate_blocks(f, 0);

	// delete file node
	memset(f, 0, sizeof(struct File));
	flush_block(f);

	/* !recycle: dir data block */
	return 0;
}

int
file_dir_each_file(struct File *dir, int (*handler)(struct File *f))
{
	int i, j, ret;
	struct File *sub_f;
	struct File **sub_fp;

	if (dir->f_type != FTYPE_DIR)
		return -E_INVAL;

	for (i = 0; i < (dir->f_size / BLKSIZE); i++) {
		if (i < NDIRECT)
			sub_f = BLKNO2ADDR(dir->f_direct[i]);
		else {
			sub_fp = BLKNO2ADDR(dir->f_indirect);
			sub_f = sub_fp[i - NDIRECT];
		}

		for (j = 0; j < BLKFILES; j++) {
			if (sub_f[j].f_name[0]) {
				ret = handler(&sub_f[j]);
				if (ret < 0)
					return ret;
			}
		}
	}

	return 0;
}

int
file_rename(struct File *dir, struct File *src_file)
{
	int ret;
	struct File *new_file;

	/* only support dst path is existing directory */
	if (dir->f_type != FTYPE_DIR)
		return -E_INVAL;

	ret = dir_alloc_file(dir, &new_file);
	if (ret < 0)
		return ret;

	memcpy(new_file, src_file, sizeof(struct File));
	flush_block(new_file);

	memset(src_file, 0, sizeof(struct File));
	flush_block(src_file);

	return 0;
}
