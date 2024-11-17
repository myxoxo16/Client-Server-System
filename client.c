#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdint.h>

#define DEFAULT_BUFFER_SIZE 4096

/* Print program usage information */
void print_usage(char *program_name) {
    printf("Usage: %s fileName IP-address:port-number [bufSize]\n", program_name);
    printf("  fileName: Path to the file to be sent\n");
    printf("  IP-address:port-number: Server IP and port\n");
    printf("  bufSize: Optional buffer size (default: 4096)\n");
}

int main(int argc, char *argv[]) {
    if (argc < 3 || argc > 4) {
        print_usage(argv[0]);
        return 1;
    }

    // Parse command-line arguments
    char *file_name = argv[1];
    char *server_address = strtok(argv[2], ":");
    char *port_str = strtok(NULL, ":");
    int buffer_size = (argc == 4) ? atoi(argv[3]) : DEFAULT_BUFFER_SIZE;

    if (!server_address || !port_str) {
        fprintf(stderr, "Invalid IP-address:port-number format\n");
        return 1;
    }

    int port = atoi(port_str);
    if (port <= 0 || buffer_size <= 0) {
        fprintf(stderr, "Invalid port or buffer size\n");
        return 1;
    }

    // Open file
    FILE *file = fopen(file_name, "rb");
    if (!file) {
        perror("Could not open file");
        return 1;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    uint64_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Create socket
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Could not create socket");
        fclose(file);
        return 1;
    }

    // Setup server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, server_address, &server_addr.sin_addr) <= 0) {
        perror("Invalid server IP address");
        close(client_socket);
        fclose(file);
        return 1;
    }

    // Connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(client_socket);
        fclose(file);
        return 1;
    }

    printf("Connected to server %s:%d\n", server_address, port);

    // Send filename length
    uint32_t filename_length = htonl(strlen(file_name));
    if (send(client_socket, &filename_length, sizeof(filename_length), 0) != sizeof(filename_length)) {
        perror("Error sending filename length");
        close(client_socket);
        fclose(file);
        return 1;
    }

    // Send filename
    if (send(client_socket, file_name, strlen(file_name), 0) != (ssize_t)strlen(file_name)) {
        perror("Error sending filename");
        close(client_socket);
        fclose(file);
        return 1;
    }

    // Send file size
    uint64_t file_size_network = htobe64(file_size);
    if (send(client_socket, &file_size_network, sizeof(file_size_network), 0) != sizeof(file_size_network)) {
        perror("Error sending file size");
        close(client_socket);
        fclose(file);
        return 1;
    }

    // Send file contents
    char buffer[buffer_size];
    size_t bytes_read, total_bytes_sent = 0;

    while ((bytes_read = fread(buffer, 1, buffer_size, file)) > 0) {
        ssize_t bytes_sent = send(client_socket, buffer, bytes_read, 0);
        if (bytes_sent < 0) {
            perror("Error sending file data");
            break;
        }
        total_bytes_sent += bytes_sent;

        // Print progress
        printf("\rSent: %lu/%lu bytes (%.2f%%)", total_bytes_sent, file_size,
               (float)total_bytes_sent * 100 / file_size);
        fflush(stdout);
    }

    printf("\nFile transfer complete!\n");

    // Cleanup
    fclose(file);
    close(client_socket);
    return 0;
}
