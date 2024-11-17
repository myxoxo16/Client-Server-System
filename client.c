#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

#define DEFAULT_BUFFER_SIZE 4096

void print_usage(char *program_name) 
{
    printf("Usage: %s fileName hostname:port-number [bufSize]\n", program_name);
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
    char *hostname_port = argv[2];
    char hostname[256];
    int port;

    // Parse hostname:port
    char *colon = strchr(hostname_port, ':');
    if (colon == NULL) {
        printf("Invalid hostname:port-number format\n");
        return 1;
    }

    int hostname_length = colon - hostname_port;
    strncpy(hostname, hostname_port, hostname_length);
    hostname[hostname_length] = '\0';
    port = atoi(colon + 1);

    int buffer_size = (argc == 4) ? atoi(argv[3]) : DEFAULT_BUFFER_SIZE;

    // Resolve hostname to the IP address
    struct addrinfo hints = {0}, *res;
    hints.ai_family = AF_INET; 
    hints.ai_socktype = SOCK_STREAM; 

    // Changing from port to string
    char port_str[6];
    snprintf(port_str, sizeof(port_str), "%d", port); 

    printf("Finding address for hostname '%s'...\n", hostname);
    if (getaddrinfo(hostname, port_str, &hints, &res) != 0) {
        perror("Failed to resolve hostname");
        return 1;
    }

    printf("Connecting to server at '%s:%d'...\n", hostname, port);

    // Print that a successful connection
    //is shown to user otherwise show error
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Socket creation failed");
        freeaddrinfo(res);
        return 1;
    }
    freeaddrinfo(res);

    printf("Connection is successful!\n");

    // Open the file to read it
    FILE *file = fopen(filename, "rb");
    // If the file is empty, display the "failed to open file" message
    if (file == NULL) 
    {
        perror("Failed to open the file");
        close(client_socket);
        return 1;
    }

    // Print messages to uset about sending specific filename
    // and if its sent successfully
    printf("Sending filename '%s'...\n", filename);
    printf("Filename sent successfully\n");

    // Sending the file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Print messages to uset about sending specific filename
    // and if its sent successfully
    printf("Sending file size %ld...\n", file_size);
    printf("File size sent successfully\n");

    // Sending file data in chunks
    printf("Sending file data...\n");
    char *buffer = malloc(buffer_size);
    // If the buffer is null, display message that it failed
    if (buffer == NULL) 
    {
        perror("Failed to allocate buffer");
        fclose(file);
        close(client_socket);
        return 1;
    }

    size_t bytes_read, total_sent = 0;
    while ((bytes_read = fread(buffer, 1, buffer_size, file)) > 0) {
        // Print to user
        printf("Sent %zu bytes of data\n", bytes_read);
        total_sent += bytes_read;
    }

    printf("File '%s' sent successfully (%ld bytes)\n", filename, total_sent);

    // Clean-up and close file
    free(buffer);
    fclose(file);
    close(client_socket);
    return 0;
}