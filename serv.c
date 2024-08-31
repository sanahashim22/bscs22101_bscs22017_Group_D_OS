#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

// PORT: The port number on which the server is listening.
// BUFFER_SIZE: The size of the buffer used to send/receive data.
#define PORT 8080
#define BUFFER_SIZE 1024
void process_file(const char *file_path)
{
    FILE *file = fopen(file_path, "r");
    char line[BUFFER_SIZE];

    if (file == NULL)
    {
        printf("Could not open file: %s\n", file_path);
        return;
    }

    char command[BUFFER_SIZE] = {0};
    char filename[BUFFER_SIZE] = {0};

    // Simple parsing, assuming JSON format {"command": "upload", "filename": "example.txt"}
    while (fgets(line, sizeof(line), file) != NULL)
    {
        if (strstr(line, "\"command\":") != NULL)
        {
            sscanf(line, " \"command\": \"%[^\"]\"", command);
        }
        else if (strstr(line, "\"filename\":") != NULL)
        {
            sscanf(line, " \"filename\": \"%[^\"]\"", filename);
        }
    }

    fclose(file);

    if (strcmp(command, "upload") == 0)
    {
        FILE *new_file = fopen(filename, "w");
        if (new_file == NULL)
        {
            printf("Could not create file: %s\n", filename);
            return;
        }

        fprintf(new_file, "This is an example content for %s\n", filename);
        fclose(new_file);
        printf("File '%s' created successfully.\n", filename);
    }
    else
    {
        printf("Unknown command: %s\n", command);
    }
}

int main()
{
    WSADATA wsa;
    SOCKET server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    /* Initialize Winsock
     WSAStartup: Initializes the Winsock library. The MAKEWORD(2,2) specifies the version of Winsock
       to use (2.2 in this case).
       If initialization fails, the program prints an error and exits. */
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("Failed. Error Code : %d\n", WSAGetLastError());
        return 1;
    }

    /*Create socket
    Sockets are used in networking to connect different computers or devices so they can communicate with each other.
    The socket() function creates a new socket. This is similar to setting up a communication channel that can be used to send or receive data.
    syntax-> int var = socket(int domain , int type, int protocol)  domain is
    internet protocol version means address family and type is which type of protocol we are using and
    protocol we set 0 because 0 is default tcp port*/
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
        printf("Could not create socket : %d\n", WSAGetLastError());
        return 1;
    }

    /* Prepare the sockaddr_in structure
    server.sin_family: Specifies the address family (AF_INET for IPv4).
       server.sin_port: Specifies the port number, converted to network byte order using htons().
       server.sin_addr.s_addr: Specifies the server's IP address (127.0.0.1 for localhost) converted to
       network byte order using inet_addr(). */
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    /*Bind
    The bind() function assigns the address specified by address to the socket.
    This is necessary so that the socket has a known address for clients to connect to.*/
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR)
    {
        printf("Bind failed with error code : %d\n", WSAGetLastError());
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    /* Listen for incoming connections
     The listen() function marks the socket as a passive socket that will be used to accept incoming connection requests using accept().*/
    listen(server_fd, 3);

    printf("Server is running and waiting for connections on port %d...\n", PORT);

    // Server loop to keep running and accepting new connections
    while (1)
    {
        printf("Waiting for a new connection...\n");

        /*Accept incoming connection
        The accept() function is used to accept an incoming connection request from a client.
        It creates a new socket (new_socket) to handle the communication with the client while leaving
        the original socket (server_fd) to continue listening for new connections.*/
        new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (new_socket == INVALID_SOCKET)
        {
            printf("Accept failed with error code : %d\n", WSAGetLastError());
            closesocket(server_fd);
            WSACleanup();
            return 1;
        }

        /*Receive a message from client
        The recv() function is used to receive data from the client.
        It reads the data into the buffer.*/
        int bytes_received = recv(new_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received > 0)
        {
            buffer[bytes_received] = '\0'; // Null-terminate the received data
            printf("Filepath from client: %s\n", buffer);
            process_file(buffer);
        }
        else if (bytes_received == 0)
        {
            printf("Client disconnected.\n");
        }
        else
        {
            printf("Receive failed with error code : %d\n", WSAGetLastError());
        }

        /*Cleanup for the current connection
        After the communication is complete, closesocket() is used to close the sockets, and WSACleanup()
        is called to clean up resources used by Winsock.*/
        closesocket(new_socket);
    }

    // Cleanup after the server stops (though this part is unlikely to be reached)
    closesocket(server_fd);
    WSACleanup();

    return 0;
}
