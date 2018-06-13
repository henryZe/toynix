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
//
// Hint: use free_block as an example for manipulating the bitmap.
int
alloc_block(void)
{
	// The bitmap consists of one or more blocks.  A single bitmap block
	// contains the in-use bits for BLKBITSIZE blocks.  There are
	// super->s_nblocks blocks in the disk altogether.

	// LAB 5: Your code here.
	panic("alloc_block not implemented");
	return -E_NO_DISK;
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

// skip over slashes
static const char *
skip_slash(const char *p)
{
	while (*p == '/')
		p++;

	return p;
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
	struct File *file;

	// Search dir for name.
	// We maintain the invariant that the size of a directory-file
	// is always a multiple of the file system's block size.
	assert((dir->f_size % BLKSIZE) == 0);
	nblock = dir->f_size / BLKSIZE;
	for (i = 0; i < nblock; i++) {
		ret = file_get_block(dir, i, &blk);
		if (ret < 0)
			return ret;
		
		file = (struct File *)blk;
		for (j = 0; j < BLKFILES; j++) {
			if (strcmp(file[j].f_name, name) == 0)
		}

	}
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

	//if (*path != '/')
	//	return -E_BAD_PATH;
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
