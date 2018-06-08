
#define ROUNDUP(n, v) ((n) - 1 + (v) - ((n) - 1) % (v))
#define MAX_DIR_ENTS 128

struct Dir {
	struct File *f;
	struct File *ents;
	int n;
};

uint32_t nblocks;
char *diskmap, *diskpos;
struct Super *super;
uint32_t *bitmap;


void
usage(void)
{
	fprintf(stderr, "Usage: fsformat fs.img NBLOCKS files\n");
	exit(2);
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

	/* for bootloader */
	alloc(BLKSIZE);

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
