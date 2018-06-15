#include <lib.h>
#include <fs/fs.h>

// --------------------------------------------------------------
// Super block
// --------------------------------------------------------------

// Validate the file system super-block.
void
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
void
free_block(uint32_t blockno)
{
	// Blockno zero is the null pointer of block numbers.
	if (blockno == 0)
		panic("attempt to free zero block");

	bitmap[blockno / 32] |= 1 << (blockno % 32);
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

	bitmap[blockno / 32] &= ~(1 << (blockno % 32));
	flush_block(&bitmap[blockno / 32]);

	return blockno;
}

// Validate the file system bitmap.
//
// Check that all reserved blocks -- 0, 1, and the bitmap blocks themselves --
// are all marked as in-use.
void
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
//
// Analogy: This is like pgdir_walk for files.
// Hint: Don't forget to clear any block you allocate.
static int
file_block_walk(struct File *f, uint32_t filebno, uint32_t **ppdiskbno, bool alloc)
{
	uint32_t blockno;

	if (blockno >= (NINDIRECT + NDIRECT))
		return -E_INVAL;

	if (filebno < NDIRECT)
		blockno = f->f_direct[filebno];
	else
		blockno = f->f_indirect[filebno - NDIRECT];




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
//
// Hint: Use file_block_walk and alloc_block.
int
file_get_block(struct File *f, uint32_t filebno, char **blk)
{



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
	char *blk;
	struct File *fp;

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

// Open "path".  On success set *pf to point at the file and return 0.
// On error return < 0.
int
file_open(const char *path, struct File **pfile)
{
	return walk_path(path, NULL, pfile, NULL);
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
			cprintf("warning: file_free_block: %e", ret);
	}

	if (new_nblocks <= NDIRECT && f->f_indirect) {
		free_block(f->f_indirect);
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
	flush_block(f);
	return 0;
}
