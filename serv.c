#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>


//PORT: The port number on which the server is listening.
//BUFFER_SIZE: The size of the buffer used to send/receive data.
#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    WSADATA wsa;
    SOCKET server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    // Initialize Winsock
     /*WSAStartup: Initializes the Winsock library. The MAKEWORD(2,2) specifies the version of Winsock 
    to use (2.2 in this case).
    If initialization fails, the program prints an error and exits.*/
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("Failed. Error Code : %d", WSAGetLastError());
        return 1;
    }

    // Create socket   
    //Sockets are used in networking to connect different computers or devices so they can communicate with each other.
    //The socket() function creates a new socket. This is similar to setting up a communication channel that can be used to send or receive data. 
    //syntax-> int var = socket(int domain , int type, int protocol)  domain is 
    //internet protocol version means address family and type is which type of protocol we are using and 
    //protocol we set 0 eacsue 00 is default tcp port
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Could not create socket : %d", WSAGetLastError());
        return 1;
    }

    // Prepare the sockaddr_in structure
    /*server.sin_family: Specifies the address family (AF_INET for IPv4).
server.sin_port: Specifies the port number, converted to network byte order using htons().
server.sin_addr.s_addr: Specifies the server's IP address (127.0.0.1 for localhost) converted to 
network byte order using inet_addr().
*/
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind
    //The bind() function assigns the address specified by address to the socket.
    //This is necessary so that the socket has a known address for clients to connect to.
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR) {
        printf("Bind failed with error code : %d", WSAGetLastError());
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    // Listen for incoming connections
    //The listen() function marks the socket as a passive socket that will be used to accept incoming connection requests using accept().
    listen(server_fd, 3);

    // Accept incoming connection
    //The accept() function is used to accept an incoming connection request from a client.
    //It creates a new socket (new_socket) to handle the communication with the client while leaving 
    //the original socket (server_fd) to continue listening for new connections.
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (int *)&addrlen)) == INVALID_SOCKET) {
        printf("Accept failed with error code : %d", WSAGetLastError());
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    // Receive a message from client
    //The recv() function is used to receive data from the client.
    //It reads the data into the buffer.
    recv(new_socket, buffer, BUFFER_SIZE, 0);
    printf("Message from client: %s\n", buffer);

    // Cleanup
    //After the communication is complete, closesocket() is used to close the sockets, and WSACleanup()
    // is called to clean up resources used by Winsock.
    closesocket(new_socket);
    closesocket(server_fd);
    WSACleanup();

    return 0;
}
