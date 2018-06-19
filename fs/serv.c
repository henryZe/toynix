#include <lib.h>
#include <x86.h>
#include <fs/fs.h>
#include <fd.h>

#define debug 0

// Max number of open files in the file system at once
#define MAXOPEN		1024
#define FILEVA		0xD0000000

// The file system server maintains three structures
// for each open file.
//
// 1. The on-disk 'struct File' is mapped into the part of memory
//    that maps the disk.  This memory is kept private to the file
//    server.
// 2. Each open file has a 'struct Fd' as well, which sort of
//    corresponds to a Unix file descriptor.  This 'struct Fd' is kept
//    on *its own page* in memory, and it is shared with any
//    environments that have the file open.
// 3. 'struct OpenFile' links these other two structures, and is kept
//    private to the file server.  The server maintains an array of
//    all open files, indexed by "file ID".  (There can be at most
//    MAXOPEN files open concurrently.)  The client uses file IDs to
//    communicate with the server.  File IDs are a lot like
//    environment IDs in the kernel.  Use openfile_lookup to translate
//    file IDs to struct OpenFile.
//
// struct Fd -> fd_file -> struct FdFile -> id -> struct OpenFile -> o_file -> struct File

struct OpenFile {
	uint32_t o_fileid;		// file id
	struct File *o_file;	// mapped descriptor for open file
	int o_mode;				// open mode
	struct Fd *o_fd;		// Fd page
};

// initialize to force into data section
struct OpenFile opentab[MAXOPEN] = {
	{ 0, 0, 1, 0 },
};

// Virtual address at which to receive page mappings containing client requests.
union Fsipc *fsreq = (union Fsipc *)0x0ffff000;

void
serve_init(void)
{
	int i;
	uintptr_t va = FILEVA;

	for (i = 0; i < MAXOPEN; i++) {
		opentab[i].o_fileid = i;
		opentab[i].o_fd = (struct Fd *)va;
		va += PGSIZE;
	}
}

// Allocate an open file.
int
openfile_alloc(struct OpenFile **o)
{
	int i, ret;

	// Find an available open-file table entry
	for (i = 0; i < MAXOPEN; i++) {
		switch (pageref(opentab[i].o_fd)) {
		case 0:
			ret = sys_page_alloc(0, opentab[i].o_fd, PTE_W);
			if (ret < 0)
				return ret;
			/* fall through */

		case 1:
			opentab[i].o_fileid += MAXOPEN;
			*o = &opentab[i];
			memset(opentab[i].o_fd, 0, PGSIZE);
			return (*o)->o_fileid;
		}
	}

	return -E_MAX_OPEN;
}

// Open req->req_path in mode req->req_omode, storing the Fd page and
// permissions to return to the calling environment in *pg_store and
// *perm_store respectively.
int
serve_open(envid_t envid, struct Fsreq_open *req,
				void **pg_store, int *perm_store)
{
	char path[MAXPATHLEN];
	struct File *f;
	int fileid, ret;
	struct OpenFile *o;

	if (debug)
		cprintf("serve_open %08x %s 0x%x\n", envid, req->req_path, req->req_omode);

	// Copy in the path, making sure it's null-terminated
	memmove(path, req->req_path, MAXPATHLEN);
	path[MAXPATHLEN - 1] = 0;

	// Find an open file ID
	ret = openfile_alloc(&o);
	if (ret < 0) {
		if (debug)
			cprintf("openfile_alloc failed: %e", ret);

		return ret;
	}
	fileid = ret;

	// Open the file
	if (req->req_omode & O_CREAT) {
		ret = file_create(path, &f);
		if (ret < 0) {
			if (!(req->req_omode & O_EXCL) && (ret == -E_FILE_EXISTS))
				goto try_open;
			if (debug)
				cprintf("file_create failed: %e", ret);

			return ret;
		}
	} else {
try_open:
		ret = file_open(path, &f);
		if (ret < 0) {
			if (debug)
				cprintf("file_open failed: %e", ret);

			return ret;
		}
	}

	// Truncate
	if (req->req_omode & O_TRUNC) {
		ret = file_set_size(f, 0);
		if (ret < 0) {
			if (debug)
				cprintf("file_set_size failed: %e", ret);

			return ret;
		}
	}

	ret = file_open(path, &f);
	if (ret < 0) {
		if (debug)
			cprintf("file_open failed: %e", ret);

		return ret;
	}

	// Save the struct file pointer
	o->o_file = f;

	// Fill out the struct Fd
	o->o_fd->fd_file.id = o->o_fileid;
	o->o_fd->fd_omode = req->req_omode & O_ACCMODE;
	o->o_fd->fd_dev_id = devfile.dev_id;
	o->o_mode = req->req_omode;

	if (debug)
		cprintf("sending success, page %08x\n", (uintptr_t)o->o_fd);

	// Share the FD page with the caller by setting *pg_store,
	// store its permission in *perm_store
	*pg_store = o->o_fd;
	*perm_store = PTE_P | PTE_U | PTE_W | PTE_SHARE;

	return 0;
}

typedef int (*fshandler)(envid_t envid, union Fsipc *req);

fshandler handlers[] = {
	// Open is handled specially because it passes pages
	[FSREQ_OPEN] = (fshandler)serve_open,
/*	[FSREQ_READ] = serve_read,
	[FSREQ_STAT] = serve_stat,
	[FSREQ_FLUSH] = (fshandler)serve_flush,
	[FSREQ_WRITE] = (fshandler)serve_write,
	[FSREQ_SET_SIZE] = (fshandler)serve_set_size,
	[FSREQ_SYNC] = serve_sync,*/
};

void
serve(void)
{
	uint32_t req;
	envid_t whom;
	int perm, ret;
	void *pg;

	while (1) {
		perm = 0;
		req = ipc_recv(&whom, fsreq, &perm);
		if (debug)
			cprintf("fs req %d from %08x [page %08x: %s]\n",
				req, whom, uvpt[PGNUM(fsreq)], fsreq);

		// All requests must contain an argument page
		if (!(perm & PTE_P)) {
			cprintf("Invalid request from %08x: no argument page\n",
				whom);
			continue; // just leave it hanging...
		}

		pg = NULL;
		if (req == FSREQ_OPEN) {
			ret = serve_open(whom, (struct Fsreq_open *)fsreq, &pg, &perm);

		} else if (req < ARRAY_SIZE(handlers) && handlers[req]) {
			ret = handlers[req](whom, fsreq);

		} else {
			cprintf("Invalid request code %d from %08x\n", req, whom);
			ret = -E_INVAL;
		}

		ipc_send(whom, ret, pg, perm);
		sys_page_unmap(0, fsreq);
	}
}

void
umain(int argc, char **argv)
{
	static_assert(sizeof(struct File) == 256);
	binaryname = "fs";
	cprintf("FS is running\n");

	// Check that we are able to do I/O
	outw(0x8A00, 0x8A00);
	cprintf("FS can do I/O\n");

	serve_init();
	fs_init();
	fs_test();
	serve();
}
