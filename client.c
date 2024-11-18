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
    //Waiting for the filename, IP-address:port-number and bufSize
    if (argc < 3 || argc > 4) 
    {
        //Print to user and quit
        printf("Usage: %s <filename> <IP-address:port-number> [bufSize]\n", argv[0]);
        return 1;
    }

    //The command line arguments for buffer size filename and ip port
    //but the buffer size as argv3 for flexibility
    char *filename = argv[1];
    char *ip_port = argv[2];
    
    //See if bufSize is given; if not, default to 4096
    int bufSize = (argc == 4) ? atoi(argv[3]) : 4096;

    if (bufSize <= 0)
    {
        //Print message to user if the size of the buffer is less or equal to 0
        fprintf(stderr, "Invalid buffer size: %d\n", bufSize);
        return 1;
    }

    //Strtok function to split strings to get correct format for the client command line
    char *server_ip = strtok(ip_port, ":");
    char *port_str = strtok(NULL, ":");
    
    if (!server_ip || !port_str)
    {
        //Display message to user
        fprintf(stderr, "Invalid IP-address:port-number format: %s\n", argv[2]);
        return 1;
    }
    
    int port = atoi(port_str);
    if (port <= 0 || port > 65535)
    {
        //Display message to user and quit
        fprintf(stderr, "Invalid port number: %s\n", port_str);
        return 1;
    }

    //Open the file and let user know
    printf("Opening file '%s'...\n", filename);
    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        //Display message to user that it cant open the file and quit
        perror("Cannot open file");
        return 1;
    }

    //Get the size of the file
    struct stat st;
    
    if (stat(filename, &st) != 0)
    {
        //Let user know that it cannot get the file size and close the 
        //file
        perror("Cannot get file size");
        fclose(file);
        return 1;
    }
    //Put the file in bytes format and print message to user
    uint64_t file_size = st.st_size;
    printf("File size: %lu bytes\n", file_size);
    //Make socket
    printf("Creating socket...\n");
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    //If the socket is less than 0, show error message to user and close
    //the file
    if (sock < 0)
    {
        perror("Socket creation failed");
        fclose(file);
        return 1;
    }

    //Set up the server address with IPv4 and
    //Convert port number
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    //Print message to user
    printf("Parsing server IP address '%s'...\n", server_ip);

    
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0)
    {
        //Print message to user
        fprintf(stderr, "Invalid server IP address format: %s\n", server_ip);
        close(sock);
        fclose(file);
        return 1;
    }

    //Tell the user that its trying to connect to the server and 
    //display the server ip and port
    printf("Connecting to server %s:%d...\n", server_ip, port);
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connect failed");
        close(sock);
        fclose(file);
        return 1;
    }
    printf("Connected to server %s:%d\n", server_ip, port);

    //Send the length of the filename
    uint32_t name_len = strlen(filename);
    name_len = htonl(name_len); 

    
    if (send(sock, &name_len, sizeof(name_len), 0) < 0)
    {
        perror("Error sending filename length");
        close(sock);
        fclose(file);
        return 1;
    }

    //Send the filename, display error message otherwise
    if (send(sock, filename, strlen(filename), 0) < 0)
    {
        perror("Error sending filename");
        close(sock);
        fclose(file);
        return 1;
    }

    //Send the size of the file
    uint64_t net_file_size = htobe64(file_size); 
    if (send(sock, &net_file_size, sizeof(net_file_size), 0) < 0)
    {
        perror("Error sending file size");
        close(sock);
        fclose(file);
        return 1;
    }

    //Send the file contents into many chunks
    printf("Sending file data...\n");
    char *buffer = malloc(bufSize);
    if (!buffer)
    {
        //Print error message to user otherwise and close 
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
    
    //Tell the user that the file is sent successfully and print the exact file
    printf("File '%s' sent successfully (%lu bytes)\n", filename, file_size);

    //Close the file, free the memory and close the socket
    //Finally, return 0
    free(buffer);
    fclose(file);
    close(sock);
    return 0;
}
