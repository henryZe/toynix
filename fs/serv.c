#include <lib.h>
#include <x86.h>
#include <fs/fs.h>

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
	struct File *o_file;		// mapped descriptor for open file
	int o_mode;			// open mode
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
			/* this case is opentab[i].o_fd never be used before */
			ret = sys_page_alloc(0, opentab[i].o_fd, PTE_W);
			if (ret < 0)
				return ret;
			/* fall through */

		case 1:
			/* this case is opentab[i].o_fd unmap by fd_close */
			/* increase the openfile counter */
			opentab[i].o_fileid += MAXOPEN;
			*o = &opentab[i];
			memset(opentab[i].o_fd, 0, PGSIZE);
			return (*o)->o_fileid;

		/* case > 1 means busy which mapped by fsipc(ipc_recv) of user process */
		}
	}

	return -E_MAX_OPEN;
}

// Look up an open file for envid.
int
openfile_lookup(envid_t envid, uint32_t fileid, struct OpenFile **po)
{
	struct OpenFile *o;

	o = &opentab[fileid % MAXOPEN];
	/* double-check openfile is using */
	if (pageref(o->o_fd) <= 1 || o->o_fileid != fileid)
		return -E_INVAL;

	*po = o;
	return 0;
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
	int ret;
	struct OpenFile *o;

	if (debug)
		cprintf("%s %08x %s 0x%x\n",
			__func__, envid, req->req_path, req->req_omode);

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

	// Open the file
	if (req->req_omode & (O_CREAT | O_MKDIR)) {
		ret = file_create(path, &f, (req->req_omode & O_MKDIR) ? true : false);
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

	if ((f->f_type == FTYPE_DIR) &&
		(req->req_omode & (O_RDWR | O_WRONLY))) {
		if (debug)
			cprintf("directory file is read-only\n");

		return -E_INVAL;
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
	/* !Notion:
	 *   PTE_SHARE indicates share the page mapping(struct Fd) with process
	 */
	*perm_store = PTE_P | PTE_U | PTE_W | PTE_SHARE;

	return 0;
}

// Set the size of req->req_fileid to req->req_size bytes, truncating
// or extending the file as necessary.
int
serve_set_size(envid_t envid, struct Fsreq_set_size *req)
{
	struct OpenFile *o;
	int ret;

	if (debug)
		cprintf("%s %08x %08x %08x\n",
			__func__, envid, req->req_fileid, req->req_size);

	// Every file system IPC call has the same general structure.
	// Here's how it goes.

	// First, use openfile_lookup to find the relevant open file.
	// On failure, return the error code to the client with ipc_send.
	ret = openfile_lookup(envid, req->req_fileid, &o);
	if (ret < 0)
		return ret;

	// Second, call the relevant file system function (from fs/fs.c).
	// On failure, return the error code to the client.
	return file_set_size(o->o_file, req->req_size);
}

// Read at most ipc->read.req_n bytes from the current seek position
// in ipc->read.req_fileid.  Return the bytes read from the file to
// the caller in ipc->readRet, then update the seek position.  Returns
// the number of bytes successfully read, or < 0 on error.
int
serve_read(envid_t envid, union Fsipc *ipc)
{
	struct Fsreq_read *req = &ipc->read;
	struct Fsret_read *req_ret = &ipc->readRet;
	struct OpenFile *o;
	int ret;

	if (debug)
		cprintf("%s %08x %08x %08x\n",
			__func__, envid, req->req_fileid, req->req_n);

	ret = openfile_lookup(envid, req->req_fileid, &o);
	if (ret < 0)
		return ret;

	ret = file_read(o->o_file, req_ret->ret_buf, req->req_n, o->o_fd->fd_offset);
	if (ret < 0)
		return ret;

	o->o_fd->fd_offset += ret;
	return ret;
}

// Write req->req_n bytes from req->req_buf to req_fileid, starting at
// the current seek position, and update the seek position
// accordingly.  Extend the file if necessary.  Returns the number of
// bytes written, or < 0 on error.
int serve_write(envid_t envid, struct Fsreq_write *req)
{
	struct OpenFile *o;
	int ret;

	if (debug)
		cprintf("%s %08x %08x %08x\n",
			__func__, envid, req->req_fileid, req->req_n);

	ret = openfile_lookup(envid, req->req_fileid, &o);
	if (ret < 0)
		return ret;

	ret = file_write(o->o_file, req->req_buf, req->req_n, o->o_fd->fd_offset);
	if (ret < 0)
		return ret;

	o->o_fd->fd_offset += ret;
	return ret;
}

// Stat ipc->stat.req_fileid.  Return the file's struct Stat to the
// caller in ipc->statRet.
int
serve_stat(envid_t envid, union Fsipc *ipc)
{
	struct Fsreq_stat *req = &ipc->stat;
	struct Fsret_stat *req_ret = &ipc->statRet;
	struct OpenFile *o;
	int ret;

	if (debug)
		cprintf("%s %08x %08x\n",
			__func__, envid, req->req_fileid);

	ret = openfile_lookup(envid, req->req_fileid, &o);
	if (ret < 0)
		return ret;

	strcpy(req_ret->ret_name, o->o_file->f_name);
	req_ret->ret_size = o->o_file->f_size;
	req_ret->ret_isdir = (o->o_file->f_type == FTYPE_DIR);
	return 0;
}

static int fileisclosed(struct File *f)
{
	int i;

	for (i = 0; i < MAXOPEN; i++) {
		// check all process attaching this file or not
		if (pageref(opentab[i].o_fd) > 1) {
			if (opentab[i].o_file == f)
				return 0;
		}
	}

	return 1;
}

// Flush all data and metadata of req->req_fileid to disk.
int serve_flush(envid_t envid, struct Fsreq_flush *req)
{
	struct OpenFile *o;
	int ret;

	if (debug)
		cprintf("%s %08x %08x\n", __func__, envid, req->req_fileid);

	ret = openfile_lookup(envid, req->req_fileid, &o);
	if (ret < 0)
		return ret;

	file_flush(o->o_file);

	// all processes detach this file
	if (fileisclosed(o->o_file))
		file_close(o->o_file);

	return 0;
}

static int file_mutex_remove(struct File *f)
{
	int ret;

	// rm sub dir
	if (f->f_type == FTYPE_DIR) {
		ret = file_dir_each_file(f, file_mutex_remove);
		if (ret < 0)
			return ret;
	}

	if (!fileisclosed(f)) {
		if (debug)
			cprintf("remove %s failed: %e\n", f->f_name, -E_BUSY);

		return -E_BUSY;
	}

	if (debug)
		cprintf("remove %s\n", f->f_name);

	ret = file_remove(f);
	if (ret < 0) {
		if (debug)
			cprintf("file_remove failed: %e", ret);

		return ret;
	}

	return 0;
}

int
serve_remove(envid_t envid, struct Fsreq_remove *req)
{
	char path[MAXPATHLEN];
	int ret;
	struct File *f;

	if (debug)
		cprintf("%s %08x %s\n", __func__, envid, req->req_path);

	// Copy in the path, making sure it's null-terminated
	memmove(path, req->req_path, MAXPATHLEN);
	path[MAXPATHLEN - 1] = 0;

	ret = file_open(path, &f);
	if (ret < 0) {
		if (debug)
			cprintf("file_open failed: %e", ret);

		return ret;
	}

	return file_mutex_remove(f);
}

int
serve_sync(envid_t envid, union Fsipc *req)
{
	fs_sync();
	return 0;
}

int
serve_info(envid_t envid, union Fsipc *req)
{
	uint32_t cnt;
	int i;

	for (cnt = 0, i = 0; i < super->s_nblocks; i++) {
		if (!block_is_free(i))
			cnt++;
	}

	req->info.blk_num = super->s_nblocks;
	req->info.blk_ocp = cnt;

	return 0;
}

int
serve_rename(envid_t envid, union Fsipc *req)
{
	int ret;
	struct File *dst_file, *src_file;

	ret = file_open(req->rename.src_path, &src_file);
	if (ret < 0)
		return ret;

	if (!fileisclosed(src_file))
		return -E_BUSY;

	ret = file_open(req->rename.dst_path, &dst_file);
	if (ret < 0)
		return ret;

	return file_rename(dst_file, src_file);
}

typedef int (*fshandler)(envid_t envid, union Fsipc *req);

fshandler handlers[] = {
	// Open is handled specially because it passes pages
	[FSREQ_OPEN] = (fshandler)serve_open,
	[FSREQ_SET_SIZE] = (fshandler)serve_set_size,
	[FSREQ_READ] = serve_read,
	[FSREQ_WRITE] = (fshandler)serve_write,
	[FSREQ_STAT] = serve_stat,
	[FSREQ_FLUSH] = (fshandler)serve_flush,
	[FSREQ_REMOVE] = (fshandler)serve_remove,
	[FSREQ_SYNC] = serve_sync,
	[FSREQ_INFO] = serve_info,
	[FSREQ_RENAME] = serve_rename,
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
	sys_env_name(0, "fs");
	cprintf("FS is running\n");

	// Check that we are able to do I/O
	outw(0x8A00, 0x8A00);
	cprintf("FS can do I/O\n");

	serve_init();
	fs_init();
	serve();
}
