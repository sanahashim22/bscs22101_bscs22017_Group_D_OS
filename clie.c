#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main()
{
    /*
    WSADATA: A structure that contains information about the Windows Sockets implementation.
SOCKET sock: A socket descriptor used to identify the socket.
struct sockaddr_in server: A structure that holds the server's address and port information.
message: The message to be sent to the server.
*/
    WSADATA wsa;
    SOCKET sock;
    struct sockaddr_in server;
    char *message = "C:\\Users\\HP\\OneDrive\\Desktop\\Semester 5\\Operating System\\Git Repo\\bscs22101_bscs22017_Group_D_OS\\cmds.txt";
    int server_addr_len = sizeof(server);

    // Initialize Winsock
    /*WSAStartup: Initializes the Winsock library. The MAKEWORD(2,2) specifies the version of Winsock
    to use (2.2 in this case).
    If initialization fails, the program prints an error and exits.*/
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("Failed. Error Code : %d", WSAGetLastError());
        return 1;
    }

    // Create socket
    /*
    socket(): Creates a new socket using:
AF_INET for IPv4 addresses.
SOCK_STREAM for a stream socket (used for TCP).
0 for the default protocol (TCP in this case).
If socket creation fails, it prints an error, cleans up Winsock, and exits.
*/
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
        printf("Could not create socket : %d", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    /*server.sin_family: Specifies the address family (AF_INET for IPv4).
    server.sin_port: Specifies the port number, converted to network byte order using htons().
    server.sin_addr.s_addr: Specifies the server's IP address (127.0.0.1 for localhost) converted to
    network byte order using inet_addr().
    */
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to server
    /*
    connect(): Attempts to establish a connection to the server using the provided server address and port.
    If the connection fails, the program prints an error, closes the socket, cleans up Winsock, and exits.
*/
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        printf("Connect failed with error code : %d", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // Send data
    /*send(): Sends the message to the server.
    If sending fails, it prints an error, closes the socket, cleans up Winsock, and exits.
    If the message is successfully sent, it prints "Message sent."*/
    if (send(sock, message, strlen(message), 0) < 0)
    {
        printf("Send failed with error code : %d", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    printf("Message sent\n");

    // Cleanup
    // closesocket(): Closes the socket, releasing the resources associated with it.
    // WSACleanup(): Cleans up the Winsock library.
    closesocket(sock);
    WSACleanup();

    return 0;
}
