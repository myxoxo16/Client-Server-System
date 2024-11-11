#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>

#define DEFAULT_BUFFER_SIZE 4096

void print_usage(char *program_name)
{
	printf("Usage: %s port-number [bufSize]\n", program_name);
	printf("  port-number: Port to listen on\n");
	printf("  bufSize: Optional buffer size (default: 4096)\n");
}

int main(int argc, char *argv[])
{
	if (argc < 2 || argc > 3)
	{
		print_usage(argv[0]);
		return 1;
	}

	// Parse arguments
	int port = atoi(argv[1]);
	int buffer_size = (argc == 3) ? atoi(argv[2]) : DEFAULT_BUFFER_SIZE;

	if (port <= 0 || buffer_size <= 0)
	{
		printf("Invalid port or buffer size\n");
		return 1;
	}

	// Create socket
	int server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == -1)
	{
		perror("Could not create socket");
		return 1;
	}

	// Set up server address structure
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(port);

	// Bind socket
	if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("Bind failed");
		close(server_socket);
		return 1;
	}

	// Listen for connections
	if (listen(server_socket, 1) < 0)
	{
		perror("Listen failed");
		close(server_socket);
		return 1;
	}

	printf("Server listening on port %d\n", port);

	// TODO: Add file receiving logic here
	// 1. Accept connection
	// 2. Receive filename
	// 3. Receive file size
	// 4. Receive file data in chunks
	// 5. Write to disk

	close(server_socket);
	return 0;
}