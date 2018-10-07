#include <lib.h>
#include <malloc.h>

#define PORT 80
#define VERSION "0.1"
#define HTTP_VERSION "1.0"

#define BUFFSIZE 512
#define MAXPENDING 5	// Max connection requests

struct http_request {
	int sock;
	char *url;
	char *version;
};

struct error_messages {
	int code;
	char *msg;
};

struct error_messages errors[] = {
	{400, "Bad Request"},
	{404, "Not Found"},
	{0, NULL},
};

struct responce_header {
	int code;
	char *header;
};

struct responce_header headers[] = {
	{ 200, 	"HTTP/" HTTP_VERSION " 200 OK\r\n"
		"Server: jhttpd/" VERSION "\r\n"},
	{ 0, NULL},
};

static void
die(char *m)
{
	cprintf("%s\n", m);
	exit();
}

// given a request, this function creates a struct http_request
static int
http_request_parse(struct http_request *req, char *request)
{
	const char *url;
	const char *version;
	int url_len, version_len;

	if (!req)
		return -1;

	if (strncmp(request, "GET ", 4))
		return -E_BAD_REQ;

	/* skip GET */
	request += 4;

	/* get the url */
	url = request;
	while (*request && *request != ' ')
		request++;
	url_len = request - url;

	req->url = malloc(url_len + 1);
	memmove(req->url, url, url_len);
	req->url[url_len] = '\0';

	/* skip space */
	request++;

	version = request;
	while (*request && *request != '\n')
		request++;
	version_len = request - version;

	req->version = malloc(version_len + 1);
	memmove(req->version, version, version_len);
	req->version[version_len] = '\0';

	// no entity parsing
	return 0;
}

static void
req_free(struct http_request *req)
{
	free(req->url);
	free(req->version);
}

static int
send_error(struct http_request *req, int code)
{
	char buf[512];
	int r;
	struct error_messages *e = errors;

	while (e->code != 0 && e->msg != 0) {
		if (e->code == code)
			break;
		e++;
	}

	if (e->code == 0)
		return -1;

	r = snprintf(buf, sizeof(buf), "HTTP/" HTTP_VERSION" %d %s\r\n"
			       "Server: jhttpd/" VERSION "\r\n"
			       "Connection: close"
			       "Content-type: text/html\r\n"
			       "\r\n"
			       "<html><body><p>%d - %s</p></body></html>\r\n",
			       e->code, e->msg, e->code, e->msg);

	if (write(req->sock, buf, r) != r)
		return -1;

	return 0;
}

static int
send_header(struct http_request *req, int code)
{
	struct responce_header *h = headers;
	while (h->code != 0 && h->header != 0) {
		if (h->code == code)
			break;
		h++;
	}

	if (h->code == 0)
		return -1;

	int len = strlen(h->header);
	if (write(req->sock, h->header, len) != len)
		die("Failed to send bytes to client");

	return 0;
}

static int
send_size(struct http_request *req, off_t size)
{
	char buf[64];
	int r;

	r = snprintf(buf, sizeof(buf), "Content-Length: %ld\r\n", (long)size);
	if (r == sizeof(buf))
		panic("buffer too small!");

	if (write(req->sock, buf, r) != r)
		return -1;

	return 0;
}

static const char*
mime_type(const char *file)
{
	//TODO: for now only a single mime type
	return "text/html";
}

static int
send_content_type(struct http_request *req)
{
	char buf[128];
	int r;
	const char *type;

	type = mime_type(req->url);
	if (!type)
		return -1;

	r = snprintf(buf, sizeof(buf), "Content-Type: %s\r\n", type);
	if (r == sizeof(buf))
		panic("buffer too small!");

	if (write(req->sock, buf, r) != r)
		return -1;

	return 0;
}

static int
send_header_fin(struct http_request *req)
{
	const char *fin = "\r\n";
	int fin_len = strlen(fin);

	if (write(req->sock, fin, fin_len) != fin_len)
		return -1;

	return 0;
}

static int
send_data(struct http_request *req, int fd)
{
	char buf[BUFFSIZE];
	int received, r;
	
	while (1) {
		received = read(fd, buf, sizeof(buf));
		if (received <= 0)
			return received;

		r = write(req->sock, buf, received);
		if (r != received)
			return -1;
	}
}

static int
send_file(struct http_request *req)
{
	int r;
	off_t file_size = -1;
	int fd;
	struct Stat st;

	// open the requested url for reading
	// if the file does not exist, send a 404 error using send_error
	// if the file is a directory, send a 404 error using send_error
	// set file_size to the size of the file
	fd = open(req->url, O_RDONLY);
	if (fd < 0)
		return send_error(req, 404);

	r = stat(req->url, &st);
	if (r < 0)
		return send_error(req, 404);

	if (st.st_isdir)
		return send_error(req, 404);

	file_size = st.st_size;

	if ((r = send_header(req, 200)) < 0)
		goto end;

	if ((r = send_size(req, file_size)) < 0)
		goto end;

	if ((r = send_content_type(req)) < 0)
		goto end;

	if ((r = send_header_fin(req)) < 0)
		goto end;

	r = send_data(req, fd);

end:
	close(fd);
	return r;
}

static void
handle_client(int sock)
{
	struct http_request con_d;
	struct http_request *req = &con_d;
	int ret;
	int received = -1;
	char buffer[BUFFSIZE];

	while (1) {
		// Receive message
		received = read(sock, buffer, BUFFSIZE);
		if (received < 0)
			panic("failed to read");

		memset(req, 0, sizeof(*req));
		req->sock = sock;

		ret = http_request_parse(req, buffer);
		if (ret == -E_BAD_REQ)
			send_error(req, 400);
		else if (ret < 0)
			panic("parse failed");
		else
			send_file(req);

		req_free(req);

		/* no keep connection alive */
		break;
	}
	close(sock);
}

void
umain(int argc, char **argv)
{
	int serversock, clientsock, ret;
	struct sockaddr_in server, client;
	
	binaryname = "jhttpd";

	// Create the TCP socket
	serversock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serversock < 0)
		die("Failed to create socket");

	// Construct the server sockaddr_in structure
	memset(&server, 0, sizeof(server));		// Clear struct
	server.sin_family = AF_INET;			// Internet/IP
	server.sin_addr.s_addr = htonl(INADDR_ANY);	// IP address
	server.sin_port = htons(PORT);			// server port

	ret = bind(serversock, (struct sockaddr *)&server, sizeof(server));
	if (ret < 0)
		die("Failed to bind the server socket");

	// Listen on the server socket
	if (listen(serversock, MAXPENDING) < 0)
		die("Failed to listen on server socket");

	cprintf("Waiting for http connections...\n");

	while (1) {
		unsigned int clientlen = sizeof(client);

		// Wait for client connection
		clientsock = accept(serversock,
					(struct sockaddr *)&client,
					&clientlen);
		if (clientsock < 0)
			die("Failed to accept client connection");

		handle_client(clientsock);
	}

	close(serversock);
}