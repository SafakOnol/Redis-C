#include <stdio.h>			// standard input-output
#include <stdlib.h>			// std library for malloc, free, exit etc.
#include <sys/socket.h>		// socket programing
#include <netinet/in.h>		// internet address family
#include <netinet/ip.h>		// ip
#include <string.h>			
#include <errno.h>			// error number macros
#include <unistd.h>			// POSIX OS API
#include <pthread.h>        // pthread library for threading

const size_t RECV_BUFFER_SIZE = 256;

static const char PING_RESPONSE[] = "+PONG\r\n";

void* handle_client(void* client_fd_ptr)
{
	int client_fd = *(int*)client_fd_ptr;
	free(client_fd_ptr); // Free the dynamically allocated memory for client_fd_ptr
	char buffer[RECV_BUFFER_SIZE];
	while (recv(client_fd, buffer, RECV_BUFFER_SIZE, 0) > 0)
	{
		send(client_fd, PING_RESPONSE, strlen(PING_RESPONSE), 0);
	}

	close(client_fd);
	return NULL;
}

int main() {
	// Disable output buffering - all output is immediately flushed and visible for debugging
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);
	
	// Confirm the program has started with a print statement
	printf("Logs from your program will appear here!\n");

	int server_fd; // server_fd: File descriptor for the server socket. 
	struct sockaddr_in client_addr; // client_addr: Structure to hold the client's address
	socklen_t client_addr_len = sizeof(client_addr); // client_addr_len: Length of the client's address structure

	server_fd = socket(AF_INET, SOCK_STREAM, 0); // AF_INET: Address family for IPv4. SOCK_STREAM: Type for TCP (connection-oriented). 0: Protocol, 0 lets the system choose the appropriate protocol for the given socket type.
	if (server_fd == -1) {
		printf("Socket creation failed: %s...\n", strerror(errno));
		return 1;
	}
	
	// Set socket options to allow reuse of local addresses (SO_REUSEADDR)
	// preventing errors like "Address already in use" when the program is restarted frequently.
	int reuse = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
		printf("SO_REUSEADDR failed: %s \n", strerror(errno));
		return 1;
	}
	
	// Define and initialize serv_addr, the server's address structure
	struct sockaddr_in serv_addr = 
	{ 
		.sin_family = AF_INET,				// sin_family: Address family, AF_INET for IPv4.
	 	.sin_port = htons(6379),			// sin_port: Port number, converted to network byte order using htons (port 6379).
	 	.sin_addr = { htonl(INADDR_ANY) },  // sin_addr: IP address, set to INADDR_ANY (bind to all available interfaces), converted to network byte order using htonl.
	 };
	
	// Bind the socket to the specified IP address and port. Check for failure and print an error message if binding fails.
	if (bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
		printf("Bind failed: %s \n", strerror(errno));
		return 1;
	}
	
	// Listen for incoming connections on the bound socket:
	int connection_backlog = 5;								// Number of incoming connections that can be queued.
	if (listen(server_fd, connection_backlog) != 0) {		// check for failure and print an error message if listening fails.
		printf("Listen failed: %s \n", strerror(errno));
		return 1;
	}
	
	// Print a message indicating the server is waiting for a client to connect and set the client_addr_len to the size of the client_addr structure.
	printf("Waiting for clients to connect...\n");
	
	while (1)
	{
		int* client_fd_ptr = malloc(sizeof(int)); // Allocate memory to store the client file descriptor
		if (!client_fd_ptr)
		{
			fprintf(stderr, "Memory allocation failed\n");
			continue;
		}

		*client_fd_ptr = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
		if (*client_fd_ptr == -1)
		{
			fprintf(stderr, "Connection error: %s\n", strerror(errno));
			free(client_fd_ptr);
			continue;
		}

		printf("Client connected\n");

		pthread_t thread_id;
		if (pthread_create(&thread_id, NULL, handle_client, client_fd_ptr) != 0)
		{
			fprintf(stderr, "Thread creation failed: %s\n", strerror(errno));
			close(*client_fd_ptr);
			free(client_fd_ptr);
			continue;
		}

		pthread_detach(thread_id); // Detach the thread to handle cleanup automatically

	}


	//// Accept an incoming connection
	//// accept blocks until a connection is present. Upon success, it returns a new socket file descriptor for the accepted connection.
	//// Define a client file descriptor to store information from accept
	//int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
	//
	//if (client_fd == -1)
	//{
	//	fprintf(stderr, "%s", "Connection error\n");
	//}

	//printf("Client connected\n");

	//char buffer[RECV_BUFFER_SIZE];
	//while (recv(client_fd, buffer, RECV_BUFFER_SIZE, 0) != 0)
	//{
	//	send(client_fd, &PING_RESPONSE, 7, 0); // 7 is hardcoded for now, from "+PONG\r\n"
	//}
	
	// Close the server socket file descriptor and return 0 to indicate successful execution.
	close(server_fd);

	return 0;
}
