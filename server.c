#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>
#include <signal.h>
#include <stdint.h>

#define DEFAULT_BUFFER_SIZE 4096
static int server_socket; // For signal handling

/* Signal handler to cleanup on ctrl+c */
static void handle_signal(int sig)
{
	printf("\nReceived signal %d, shutting down...\n", sig);
	close(server_socket);
	exit(0);
}

/* Handles duplicate filenames by appending (2), (3), etc. */
char *get_unique_filename(const char *original_filename)
{
	char *new_filename = strdup(original_filename);
	char *ext = strrchr(new_filename, '.');
	char base[256];
	char extension[32] = "";

	// Split filename into base and extension
	if (ext != NULL)
	{
		strncpy(extension, ext, sizeof(extension) - 1);
		*ext = '\0';
		strncpy(base, new_filename, sizeof(base) - 1);
	}
	else
	{
		strncpy(base, new_filename, sizeof(base) - 1);
	}

	// Try to open the file to check if it exists
	FILE *file = fopen(original_filename, "r");
	if (file == NULL)
	{
		// File doesn't exist, use original name
		free(new_filename);
		return strdup(original_filename);
	}
	fclose(file);

	// File exists, try with numbers
	int counter = 2;
	char temp[512];
	do
	{
		snprintf(temp, sizeof(temp), "%s(%d)%s", base, counter, extension);
		file = fopen(temp, "r");
		if (file == NULL)
		{
			// Found a unique name
			free(new_filename);
			return strdup(temp);
		}
		fclose(file);
		counter++;
	} while (counter < 100); // Limit to prevent infinite loop

	free(new_filename);
	return NULL;
}

/* Print program usage information */
void print_usage(char *program_name)
{
	printf("Usage: %s port-number [bufSize]\n", program_name);
	printf("  port-number: Port to listen on\n");
	printf("  bufSize: Optional buffer size (default: 4096)\n");
}

int main(int argc, char *argv[])
{
	// Validate command line arguments
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

	// Setup signal handling for clean shutdown
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handle_signal;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	// Socket setup and configuration
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == -1)
	{
		perror("Could not create socket");
		return 1;
	}

	// Set socket options
	int flag = 1;
	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) == -1)
	{
		perror("setsockopt() failed");
		close(server_socket);
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

	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);

	// Main server loop - handle clients sequentially
	while (1)
	{
		// Wait for and accept client connection
		printf("Waiting for connections...\n");

		// Accept connection
		int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
		if (client_socket < 0)
		{
			perror("Accept failed");
			continue; // Continue listening for other connections
		}

		// Print client information
		printf("Connection from %s:%d\n",
			   inet_ntoa(client_addr.sin_addr),
			   ntohs(client_addr.sin_port));

		char buffer[buffer_size];
		uint32_t filename_length;
		uint64_t file_size;

		// Receive filename length
		ssize_t bytes_read = recv(client_socket, &filename_length, sizeof(filename_length), 0);
		if (bytes_read != sizeof(filename_length))
		{
			perror("Error receiving filename length");
			close(client_socket);
			continue;
		}

		// Convert from network byte order
		filename_length = ntohl(filename_length);

		// Receive filename
		char filename[256]; // Max filename length
		if (filename_length > sizeof(filename) - 1)
		{
			fprintf(stderr, "Filename too long\n");
			close(client_socket);
			continue;
		}

		bytes_read = recv(client_socket, filename, filename_length, 0);
		if (bytes_read != filename_length)
		{
			perror("Error receiving filename");
			close(client_socket);
			continue;
		}
		filename[filename_length] = '\0';

		// Receive file size
		bytes_read = recv(client_socket, &file_size, sizeof(file_size), 0);
		if (bytes_read != sizeof(file_size))
		{
			perror("Error receiving file size");
			close(client_socket);
			continue;
		}

		// Convert from network byte order
		file_size = be64toh(file_size);

		printf("Receiving file: %s (size: %lu bytes)\n", filename, file_size);

		// Get unique filename
		char *unique_filename = get_unique_filename(filename);
		if (unique_filename == NULL)
		{
			fprintf(stderr, "Could not create unique filename\n");
			close(client_socket);
			continue;
		}

		// Open file for writing
		FILE *file = fopen(unique_filename, "wb");
		if (file == NULL)
		{
			perror("Error opening file for writing");
			free(unique_filename);
			close(client_socket);
			continue;
		}

		if (strcmp(unique_filename, filename) != 0)
		{
			printf("File already exists. Saving as: %s\n", unique_filename);
		}

		// Receive file contents in chunks
		uint64_t total_bytes_received = 0;
		ssize_t chunk_size;

		while (total_bytes_received < file_size)
		{
			// Calculate remaining bytes to receive
			size_t remaining = file_size - total_bytes_received;
			size_t to_receive = (remaining < buffer_size) ? remaining : buffer_size;

			// Receive chunk
			chunk_size = recv(client_socket, buffer, to_receive, 0);
			if (chunk_size <= 0)
			{
				if (chunk_size == 0)
				{
					printf("Connection closed by client\n");
				}
				else
				{
					perror("Error receiving file data");
				}
				break;
			}

			// Write chunk to file
			size_t written = fwrite(buffer, 1, chunk_size, file);
			if (written != chunk_size)
			{
				perror("Error writing to file");
				break;
			}

			total_bytes_received += chunk_size;

			// Print progress
			printf("\rReceived: %lu/%lu bytes (%.2f%%)",
				   total_bytes_received, file_size,
				   (float)total_bytes_received * 100 / file_size);
			fflush(stdout);
		}

		// Close the file
		fclose(file);

		// Print transfer summary
		if (total_bytes_received == file_size)
		{
			printf("\nTransfer complete!\n");
			printf("Summary:\n");
			printf("  Original filename: %s\n", filename);
			printf("  Saved as: %s\n", unique_filename);
			printf("  Size: %lu bytes\n", file_size);
			printf("  Client IP: %s\n", inet_ntoa(client_addr.sin_addr));
			printf("  Buffer size: %d bytes\n", buffer_size);
		}
		else
		{
			printf("\nTransfer incomplete! Received %lu/%lu bytes\n",
				   total_bytes_received, file_size);
			// Remove incomplete file
			remove(unique_filename);
		}

		free(unique_filename);
		close(client_socket);
	}

	close(server_socket);
	return 0;
}