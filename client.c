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
    if (argc != 4) // Expect filename, server IP, and port
    {
        printf("Usage: %s <filename> <server IP> <port>\n", argv[0]);
        return 1;
    }

    // Extract arguments
    char *filename = argv[1];
    char *server_ip = argv[2];
    int port = atoi(argv[3]);

    // Open the file to send
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

    // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
<<<<<<< HEAD
    if (sock < 0)
=======

    // Connect to server (localhost:8080) get ip port, buffer size from command line
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
>>>>>>> ee35470433494c6f988f38938edbd72a30ab835b
    {
        perror("Socket creation failed");
        fclose(file);
        return 1;
    }

    // Connect to server
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    int result = inet_pton(AF_INET, server_ip, &server_addr.sin_addr);
    if (result == 0)
    {
        fprintf(stderr, "Invalid server IP address format: %s\n", server_ip);
        close(sock);
        fclose(file);
        return 1;
    }
    else if (result < 0)
    {
        perror("inet_pton failed");
        close(sock);
        fclose(file);
        return 1;
    }

    printf("Attempting to connect to server %s:%d...\n", server_ip, port);
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
    char buffer[4096];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        if (send(sock, buffer, bytes_read, 0) < 0)
        {
            perror("Error sending file data");
            close(sock);
            fclose(file);
            return 1;
        }
    }

<<<<<<< HEAD
    printf("File '%s' sent successfully (%lu bytes)\n", filename, file_size);

=======
>>>>>>> ee35470433494c6f988f38938edbd72a30ab835b
    fclose(file);
    close(sock);
    return 0;
}
