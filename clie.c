#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     // For close()
#include <arpa/inet.h>  // For inet_addr() and htons()
#include <sys/socket.h> // For socket(), connect(), send(), recv()

#define PORT 8001
#define BUFFER_SIZE 2048

int main()
{
    int sock;
    struct sockaddr_in server;
    char *message = "/home/haris/Desktop/OS/Codes/command.txt";
    char server_response[BUFFER_SIZE] = {0};
    char file_content[BUFFER_SIZE] = {0};

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation failed");
        return 1;
    }

    // Setup server address structure
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to server
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("Connection failed");
        close(sock);
        return 1;
    }

    // Send the command file path to the server
    if (send(sock, message, strlen(message), 0) < 0)
    {
        perror("Send failed");
        close(sock);
        return 1;
    }

    printf("File path sent to server.\n");

    // Receive server response regarding disk space
    // Receive server response
    int bytes_received = recv(sock, server_response, BUFFER_SIZE, 0);
    if (bytes_received > 0)
    {
        server_response[bytes_received] = '\0';
        // printf("Server response: %s\n", server_response);

        // Check if the response contains file content or a failure message
        if (strstr(server_response, "Success: Ready to receive file content.") != NULL)
        {
            // Server is ready to receive the file content
            printf("Enter the content for the file:\n");
            fgets(file_content, BUFFER_SIZE, stdin);

            // Send the file content to the server
            if (send(sock, file_content, strlen(file_content), 0) < 0)
            {
                perror("Send failed");
            }
            else
            {
                printf("File content sent to server.\n");
            }
        }
        else if (strstr(server_response, "File content:") == NULL)
        {
            // Print the file content received from the servers
            printf("File content received from server:\n%s\n", server_response);
        }
        else if (strstr(server_response, "Failure:") == NULL)
        {
            // Print the failure message
            printf("Server response: %s\n", server_response);
        }
        else if (strstr(server_response, "Files in directory:") == NULL)
        {
            // Print the list of files with details (for "view" command)
            printf("Files in directory received from server:\n%s\n", server_response);
        }
    }
    else
    {
        printf("Failed to receive response from server\n");
    }

    // Cleanup
    close(sock);

    return 0;
}