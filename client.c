#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>

int main(int argc, char *argv[])
{
    if (argc != 4) // Expect filename, IP-address:port-number, bufSize
    {
        printf("Usage: %s <filename> <IP-address:port-number> <bufSize>\n", argv[0]);
        return 1;
    }

    // Extract arguments
    char *filename = argv[1];
    char *ip_port = argv[2];
    int bufSize = atoi(argv[3]); // Buffer size

    if (bufSize <= 0)
    {
        fprintf(stderr, "Invalid buffer size: %d\n", bufSize);
        return 1;
    }

    // Split IP and port from ip_port (e.g., "127.0.0.1:8080")
    char *server_ip = strtok(ip_port, ":");
    char *port_str = strtok(NULL, ":");
    if (!server_ip || !port_str)
    {
        fprintf(stderr, "Invalid IP-address:port-number format: %s\n", argv[2]);
        return 1;
    }
    int port = atoi(port_str);
    if (port <= 0 || port > 65535)
    {
        fprintf(stderr, "Invalid port number: %s\n", port_str);
        return 1;
    }

    // Open the file to send
    printf("Opening file '%s'...\n", filename);
    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        perror("Cannot open file");
        return 1;
    }

    // Get file size
    struct stat st;
    if (stat(filename, &st) != 0)
    {
        perror("Cannot get file size");
        fclose(file);
        return 1;
    }
    uint64_t file_size = st.st_size;
    printf("File size: %lu bytes\n", file_size);

    // Create socket
    printf("Creating socket...\n");
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("Socket creation failed");
        fclose(file);
        return 1;
    }

    // Configure server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    printf("Parsing server IP address '%s'...\n", server_ip);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0)
    {
        fprintf(stderr, "Invalid server IP address format: %s\n", server_ip);
        close(sock);
        fclose(file);
        return 1;
    }

    // Connect to server
    printf("Connecting to server %s:%d...\n", server_ip, port);
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connect failed");
        close(sock);
        fclose(file);
        return 1;
    }
    printf("Connected to server %s:%d\n", server_ip, port);

    // Send filename length
    uint32_t name_len = strlen(filename);
    name_len = htonl(name_len); // Convert to network byte order
    if (send(sock, &name_len, sizeof(name_len), 0) < 0)
    {
        perror("Error sending filename length");
        close(sock);
        fclose(file);
        return 1;
    }

    // Send filename
    if (send(sock, filename, strlen(filename), 0) < 0)
    {
        perror("Error sending filename");
        close(sock);
        fclose(file);
        return 1;
    }

    // Send file size
    uint64_t net_file_size = htobe64(file_size); // Convert to network byte order
    if (send(sock, &net_file_size, sizeof(net_file_size), 0) < 0)
    {
        perror("Error sending file size");
        close(sock);
        fclose(file);
        return 1;
    }

    // Send file content
    printf("Sending file data...\n");
    char *buffer = malloc(bufSize);
    if (!buffer)
    {
        fprintf(stderr, "Failed to allocate buffer of size %d\n", bufSize);
        close(sock);
        fclose(file);
        return 1;
    }

    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, bufSize, file)) > 0)
    {
        if (send(sock, buffer, bytes_read, 0) < 0)
        {
            perror("Error sending file data");
            free(buffer);
            close(sock);
            fclose(file);
            return 1;
        }
    }

    printf("File '%s' sent successfully (%lu bytes)\n", filename, file_size);

    free(buffer);
    fclose(file);
    close(sock);
    return 0;
}
