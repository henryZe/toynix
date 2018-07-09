#include <lib.h>




static int
alloc_sockfd()
{

}

int
socket(int domain, int type, int protocol)
{
	int ret;

	ret = nsipc_socket(domain, type, protocol);
	if (ret < 0)
		return ret;

	return alloc_sockfd(ret);
}





struct Dev devsock = {
	.dev_id = 's',
	.dev_name =	"sock",
//	.dev_read =	devsock_read,
//	.dev_write = devsock_write,
//	.dev_close = devsock_close,
//	.dev_stat =	devsock_stat,
};