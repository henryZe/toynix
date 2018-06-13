/* Image Format Tool */

// We don't actually want to define off_t!
#define off_t xxx_off_t
#define bool xxx_bool
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#undef off_t
#undef bool

// Prevent include/types.h, included from include/fs.h,
// from attempting to redefine types defined in the host's inttypes.h.
// Typedef the types that include/mmu.h needs.
typedef uint32_t physaddr_t;
typedef uint32_t off_t;
typedef int bool;

/* include toynix header file */
#define FS_FORMAT_TOOL
#include <include/mmu.h>
#include <include/fs.h>

#define ROUNDUP(n, v) ((n) - 1 + (v) - ((n) - 1) % (v))
#define MAX_DIR_ENTS 128

struct Dir {
	struct File *f;
	struct File *ents;
	int n;	/* num of files within Dir */
};

uint32_t nblocks;
char *diskmap, *diskpos;
struct Super *super;
uint32_t *bitmap;

void
panic(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
	abort();
}

void
usage(void)
{
	fprintf(stderr, "Usage: fsformat fs.img NBLOCKS files\n");
	exit(2);
}

uint32_t
blockof(void *pos)
{
	return ((char *)pos - diskmap) / BLKSIZE;
}

void *
alloc(uint32_t bytes)
{
	void *start = diskpos;

	diskpos += ROUNDUP(bytes, BLKSIZE);
	if (blockof(diskpos) >= nblocks)
		panic("out of disk blocks");

	return start;
}

void
opendisk(const char *name)
{
	int ret, diskfd, nbitblocks;

	diskfd = open(name, O_RDWR | O_CREAT, 0666);
	if (diskfd < 0)
		panic("open %s: %s", name, strerror(errno));

	ret = ftruncate(diskfd, 0);
	if (ret < 0)
		panic("truncate %s: %s", name, strerror(errno));

	ret = ftruncate(diskfd, nblocks * BLKSIZE);
	if (ret < 0)
		panic("truncate %s: %s", name, strerror(errno));

	diskmap = mmap(NULL, nblocks * BLKSIZE, PROT_READ | PROT_WRITE,
				MAP_SHARED, diskfd, 0);
	if (diskmap == MAP_FAILED)
		panic("mmap %s: %s", name, strerror(errno));

	close(diskfd);

	diskpos = diskmap;

	/* block0 for boot sector, partition table */
	alloc(BLKSIZE);

	/* block1 for superblock */
	super = alloc(BLKSIZE);
	super->s_magic = FS_MAGIC;
	super->s_nblocks = nblocks;
	super->s_root.f_type = FTYPE_DIR;
	strcpy(super->s_root.f_name, "/");

	/*
	 * nblocks = Roundup(nblocks, BLKBITSIZE)
	 * nbitblocks = nblocks / (BLKSIZE * 8)
	 */
	nbitblocks = (nblocks + BLKBITSIZE - 1) / BLKBITSIZE;
	bitmap = alloc(nbitblocks * BLKSIZE);
	memset(bitmap, 0xFF, nbitblocks * BLKSIZE);
}

void
startdir(struct File *file, struct Dir *dout)
{
	dout->f = file;
	dout->ents = malloc(MAX_DIR_ENTS * sizeof(*dout->ents));
	dout->n = 0;
}

struct File *
diradd(struct Dir *d, uint32_t type, const char *name)
{
	struct File *out = &d->ents[d->n++];

	if (d->n > MAX_DIR_ENTS)
		panic("too many directory entries");

	strcpy(out->f_name, name);
	out->f_type = type;
	return out;
}

void
readn(int fd, void *out, size_t n)
{
	size_t p = 0;
	while (p < n) {
		ssize_t m = read(fd, out + p, n - p);
		if (m < 0)
			panic("read: %s", strerror(errno));
		if (m == 0)
			panic("read: Unexpected EOF");
		p += m;
	}
}

void
finishfile(struct File *file, uint32_t start_blk, uint32_t len)
{
	int i;

	file->f_size = len;
	len = ROUNDUP(len, BLKSIZE);
	for (i = 0; i < len / BLKSIZE && i < NDIRECT; ++i)
		file->f_direct[i] = start_blk + i;
	if (i == NDIRECT) {
		uint32_t *index = alloc(BLKSIZE);
		file->f_indirect = blockof(index);
		for (; i < len / BLKSIZE; ++i)
			index[i - NDIRECT] = start_blk + i;
	}
}

void
writefile(struct Dir *dir, const char *name)
{
	int ret, fd;
	struct File *file;
	struct stat st;
	const char *last;
	char *start;

	fd = open(name, O_RDONLY);
	if (fd < 0)
		panic("open %s: %s", name, strerror(errno));

	ret = fstat(fd, &st);
	if (ret < 0)
		panic("stat %s: %s", name, strerror(errno));

	if (!S_ISREG(st.st_mode))
		panic("%s is not a regular file", name);

	if (st.st_size >= MAXFILESIZE)
		panic("%s too large", name);

	last = strrchr(name, '/');
	if (last)
		last++;
	else
		last = name;

	file = diradd(dir, FTYPE_REG, last);
	start = alloc(st.st_size);
	readn(fd, start, st.st_size);
	finishfile(file, blockof(start), st.st_size);
	close(fd);
}

void
finishdir(struct Dir *d)
{
	int size = d->n * sizeof(struct File);
	struct File *start = alloc(size);
	memmove(start, d->ents, size);
	finishfile(d->f, blockof(start), ROUNDUP(size, BLKSIZE));
	free(d->ents);
	d->ents = NULL;
}

void
finishdisk(void)
{
	int ret, i;
	for (i = 0; i < blockof(diskpos); ++i)
		/* clear block-free bit */
		bitmap[i / 32] &= ~(1 << (i % 32));

	/* wait for sync whole memory */
	ret = msync(diskmap, nblocks * BLKSIZE, MS_SYNC);
	if (ret < 0)
		panic("msync: %s", strerror(errno));
}

int
main(int argc, char **argv)
{
	int i;
	char *s;
	struct Dir root;

	assert(BLKSIZE % sizeof(struct File) == 0);

	if (argc < 3)
		usage();

	nblocks = strtol(argv[2], &s, 0);
	if (*s || s == argv[2] || nblocks < 2 || nblocks > 1024)
		usage();

	opendisk(argv[1]);

	startdir(&super->s_root, &root);
	for (i = 3; i < argc; i++)
		writefile(&root, argv[i]);
	finishdir(&root);

	finishdisk();
	return 0;
}