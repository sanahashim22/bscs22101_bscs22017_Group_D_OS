#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// Function to check available disk space in bytes
unsigned long long get_free_space(const char *drive)
{
    ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;
    if (GetDiskFreeSpaceEx(drive, &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes))
    {
        return freeBytesAvailable.QuadPart;
    }
    return 0;
}

// Function to process the file based on the command
void process_file(const char *file_path, SOCKET client_socket)
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
    char drive[4] = {0}; // For drive letter, e.g., "C:"

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
        // Extract drive letter from filename
        strncpy(drive, filename, 2);
        drive[2] = '\0'; // Null-terminate

        // Check if there is enough space
        unsigned long long free_space = get_free_space(drive);
        printf("Free space on drive %s: %llu bytes\n", drive, free_space);

        if (free_space > 100000) // Check if there is more than 100KB free space
        {
            // Send success message to client
            char success_message[] = "Success: Ready to receive file content.";
            send(client_socket, success_message, strlen(success_message), 0);

            // Receive file content from client
            char file_content[BUFFER_SIZE] = {0};
            int bytes_received = recv(client_socket, file_content, BUFFER_SIZE, 0);
            if (bytes_received > 0)
            {
                file_content[bytes_received] = '\0';
                printf("Received file content:\n%s", file_content);

                // Write the received content to the file
                FILE *new_file = fopen(filename, "w");
                if (new_file == NULL)
                {
                    printf("Could not create file: %s\n", filename);
                    return;
                }

                fprintf(new_file, "%s", file_content);
                fclose(new_file);
                printf("File '%s' created successfully.\n", filename);
            }
        }
        else
        {
            // Send failure message to client
            char failure_message[] = "Failure: Not enough disk space.";
            send(client_socket, failure_message, strlen(failure_message), 0);
            printf("Not enough disk space for file: %s\n", filename);
        }
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

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("Failed. Error Code : %d\n", WSAGetLastError());
        return 1;
    }

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
        printf("Could not create socket : %d\n", WSAGetLastError());
        return 1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR)
    {
        printf("Bind failed with error code : %d\n", WSAGetLastError());
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    listen(server_fd, 3);

    printf("Server is running and waiting for connections on port %d...\n", PORT);

    while (1)
    {
        printf("Waiting for a new connection...\n");

        new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (new_socket == INVALID_SOCKET)
        {
            printf("Accept failed with error code : %d\n", WSAGetLastError());
            closesocket(server_fd);
            WSACleanup();
            return 1;
        }

        int bytes_received = recv(new_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received > 0)
        {
            buffer[bytes_received] = '\0';
            printf("Filepath from client: %s\n", buffer);
            process_file(buffer, new_socket);
        }
        else if (bytes_received == 0)
        {
            printf("Client disconnected.\n");
        }
        else
        {
            printf("Receive failed with error code : %d\n", WSAGetLastError());
        }

        closesocket(new_socket);
    }

    closesocket(server_fd);
    WSACleanup();

    return 0;
}
