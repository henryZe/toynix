#include <types.h>
#include <fd.h>

#define debug 0

struct Dev devfile = {
	.dev_id = 'f',
	.dev_name = "file",
/*	.dev_read = devfile_read,
	.dev_close = devfile_flush,
	.dev_stat = devfile_stat,
	.dev_write = devfile_write,
	.dev_trunc = devfile_trunc,
*/
};



