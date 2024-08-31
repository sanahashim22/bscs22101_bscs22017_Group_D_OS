#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main()
{
    WSADATA wsa;
    SOCKET sock;
    struct sockaddr_in server;
    char *message = "C:\\Users\\HP\\OneDrive\\Desktop\\Semester 5\\Operating System\\Git Repo\\bscs22101_bscs22017_Group_D_OS\\cmds.txt";
    char server_response[BUFFER_SIZE] = {0};
    char file_content[BUFFER_SIZE] = {0};

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("Failed. Error Code : %d", WSAGetLastError());
        return 1;
    }

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
        printf("Could not create socket : %d", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Setup server address structure
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to server
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        printf("Connect failed with error code : %d", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // Send the command file path to the server
    if (send(sock, message, strlen(message), 0) < 0)
    {
        printf("Send failed with error code : %d", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    printf("File path sent to server.\n");

    // Receive server response regarding disk space
    int bytes_received = recv(sock, server_response, BUFFER_SIZE, 0);
    if (bytes_received > 0)
    {
        server_response[bytes_received] = '\0';
        printf("Server response: %s\n", server_response);

        if (strstr(server_response, "Success") != NULL)
        {
            // Server is ready to receive the file content
            printf("Enter the content for the file:\n");
            fgets(file_content, BUFFER_SIZE, stdin);

            // Send the file content to the server
            if (send(sock, file_content, strlen(file_content), 0) < 0)
            {
                printf("Send failed with error code : %d", WSAGetLastError());
            }
            else
            {
                printf("File content sent to server.\n");
            }
        }
        else
        {
            printf("Server could not process the file due to insufficient space.\n");
        }
    }
    else
    {
        printf("Failed to receive response from server. Error code : %d\n", WSAGetLastError());
    }

    // Cleanup
    closesocket(sock);
    WSACleanup();

    return 0;
}
