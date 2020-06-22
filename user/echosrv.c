#include <lib.h>

#define PORT 7

#define BUFFSIZE 32
#define MAXPENDING 5    // Max connection requests

static void
die(char *m)
{
	cprintf("%s\n", m);
	exit();
}

static void
handle_client(int sock)
{
	char buffer[BUFFSIZE];
	int received = -1;

	// Receive message
	received = read(sock, buffer, BUFFSIZE);
	if (received < 0)
		die("Failed to receive initial bytes from client");

	// Send bytes and check for more incoming data in loop
	while (received > 0) {
		// Send back received data
		if (write(sock, buffer, received) != received)
			die("Failed to send bytes to client");

		// Check for more data
		received = read(sock, buffer, BUFFSIZE);
		if (received < 0)
			die("Failed to receive additional bytes from client");
	}
	close(sock);
}

void
umain(int argc, char **argv)
{
	int ret, serversock, clientsock;
	struct sockaddr_in echoserver, echoclient;

	// Create the TCP socket
	serversock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serversock < 0)
		die("Failed to create socket");

	cprintf("opened socket\n");

	// Construct the server sockaddr_in structure
	memset(&echoserver, 0, sizeof(echoserver));       // Clear struct
	echoserver.sin_family = AF_INET;                  // Internet/IP
	echoserver.sin_addr.s_addr = htonl(INADDR_ANY);   // IP address
	echoserver.sin_port = htons(PORT);			// server port

	cprintf("trying to bind\n");

	ret = bind(serversock, (struct sockaddr *) &echoserver, sizeof(echoserver));
	if (ret < 0)
		die("Failed to bind the server socket");

	// Listen on the server socket
	if (listen(serversock, MAXPENDING) < 0)
		die("Failed to listen on server socket");

	cprintf("bound\n");

	// Run until canceled
	while (1) {
		unsigned int clientlen = sizeof(echoclient);
		// Wait for client connection
		clientsock = accept(serversock, (struct sockaddr *) &echoclient, &clientlen);
		if (clientsock < 0)
			die("Failed to accept client connection");

		cprintf("Client connected: %s\n", inet_ntoa(echoclient.sin_addr));
		handle_client(clientsock);
	}

	close(serversock);
}

