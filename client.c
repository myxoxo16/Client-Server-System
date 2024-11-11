#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define DEFAULT_BUFFER_SIZE 4096

void print_usage(char *program_name)
{
	printf("Usage: %s fileName IP-address:port-number [bufSize]\n", program_name);
}

int main(int argc, char *argv[])
{
	if (argc < 3 || argc > 4)
	{
		print_usage(argv[0]);
		return 1;
	}

	// Parse arguments
	char *filename = argv[1];
	char *ip_port = argv[2];
	char ip_address[16];
	int port;

	// Parse IP:port
	char *colon = strchr(ip_port, ':');
	if (colon == NULL)
	{
		printf("Invalid IP-address:port-number format\n");
		return 1;
	}

	int ip_length = colon - ip_port;
	strncpy(ip_address, ip_port, ip_length);
	ip_address[ip_length] = '\0';
	port = atoi(colon + 1);

	int buffer_size = (argc == 4) ? atoi(argv[3]) : DEFAULT_BUFFER_SIZE;

	// Create socket
	int client_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (client_socket == -1)
	{
		perror("Could not create socket");
		return 1;
	}

	// Set up server address structure
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);

	if (inet_pton(AF_INET, ip_address, &server_addr.sin_addr) <= 0)
	{
		perror("Invalid address");
		close(client_socket);
		return 1;
	}

	// Connect to server
	if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("Connect failed");
		close(client_socket);
		return 1;
	}

	// TODO: Add file sending logic here
	// 1. Open and read file
	// 2. Send filename
	// 3. Send file size
	// 4. Send file data in chunks

	close(client_socket);
	return 0;
}