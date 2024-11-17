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
    if (argc != 2)
    {
        printf("Usage: %s filename\n", argv[0]);
        return 1;
    }

    // Open the file to send
    FILE *file = fopen(argv[1], "rb");
    if (!file)
    {
        perror("Cannot open file");
        return 1;
    }

    // Get file size
    struct stat st;
    stat(argv[1], &st);
    uint64_t file_size = st.st_size;

    // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    // Connect to server (localhost:8080) get ip port, buffer size from command line
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connect failed");
        return 1;
    }

    // Send filename length
    uint32_t name_len = strlen(argv[1]);
    name_len = htonl(name_len); // Convert to network byte order
    send(sock, &name_len, sizeof(name_len), 0);

    // Send filename
    send(sock, argv[1], strlen(argv[1]), 0);

    // Send file size
    uint64_t net_file_size = htobe64(file_size); // Convert to network byte order
    send(sock, &net_file_size, sizeof(net_file_size), 0);

    // Send file content
    char buffer[4096];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        send(sock, buffer, bytes_read, 0);
    }

    fclose(file);
    close(sock);
    return 0;
}
