#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// Function to check available disk space in bytes
unsigned long long get_free_space(const char *path)
{
    struct statvfs stat;

    if (statvfs(path, &stat) != 0)
    {
        // Error getting filesystem statistics
        return 0;
    }

    // Calculate the available space in bytes
    return stat.f_bsize * stat.f_bavail;
}

// Function to process the file based on the command
void process_file(const char *file_path, int client_socket)
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
    char path[BUFFER_SIZE] = "/"; // Default to root for checking disk space
    char filepath[BUFFER_SIZE] = {0};

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
        else if (strstr(line, "\"filepath\":") != NULL)
        {
            sscanf(line, " \"filepath\": \"%[^\"]\"", filepath);
        }
    }

    fclose(file);

    if (strcmp(command, "upload") == 0)
    {
        // Use the root ("/") for checking free space
        unsigned long long free_space = get_free_space(path);
        printf("Free space on path %s: %llu bytes\n", path, free_space);

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
    else if (strcmp(command, "view") == 0)
    {
        DIR *dir;
        struct dirent *entry;
        struct stat file_stat;
        char response[BUFFER_SIZE] = "Files:\n";
        int files_found = 0;

        // Open the directory where client files are stored
        dir = opendir(".");
        if (dir == NULL)
        {
            perror("opendir");
            return;
        }

        // Iterate over the directory entries
        while ((entry = readdir(dir)) != NULL)
        {
            // Use stat() to get file information
            if (stat(entry->d_name, &file_stat) == 0 && S_ISREG(file_stat.st_mode))
            {
                files_found = 1;

                // Append file info to the response
                char file_info[BUFFER_SIZE];
                snprintf(file_info, sizeof(file_info), "Name: %s, Size: %lld bytes, Last Modified: %s",
                         entry->d_name, (long long)file_stat.st_size, ctime(&file_stat.st_mtime));
                strcat(response, file_info);
            }
        }
        closedir(dir);

        if (files_found)
        {
            send(client_socket, response, strlen(response), 0);
        }
        else
        {
            char failure_message[] = "Failure: No files found for the client.";
            send(client_socket, failure_message, strlen(failure_message), 0);
            printf("No files found for the client.\n");
        }
    }
    else if (strcmp(command, "download") == 0)
    {
        char file_content[BUFFER_SIZE];
        FILE *file;
        int bytes_read;

        // Open the file
        file = fopen(filepath, "r");
        if (file == NULL)
        {
            // File not found
            char failure_message[] = "Failure: File not found.";
            send(client_socket, failure_message, strlen(failure_message), 0);
            printf("File '%s' not found.\n", filepath);
            return;
        }

        // Send the file content to the client
        while ((bytes_read = fread(file_content, 1, sizeof(file_content) - 1, file)) > 0)
        {
            file_content[bytes_read] = '\0';
            send(client_socket, file_content, bytes_read, 0);
        }

        fclose(file);
        printf("File '%s' sent to client.\n", filepath);
    }
    else
    {
        printf("Unknown command: %s\n", command);
    }
}

int main()
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation failed");
        return 1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind socket to the port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        close(server_fd);
        return 1;
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0)
    {
        perror("Listen failed");
        close(server_fd);
        return 1;
    }

    printf("Server is running and waiting for connections on port %d...\n", PORT);

    while (1)
    {
        printf("Waiting for a new connection...\n");

        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_socket < 0)
        {
            perror("Accept failed");
            close(server_fd);
            return 1;
        }

        int bytes_received = recv(new_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received > 0)
        {
            buffer[bytes_received] = '\0';
            printf("Received command: %s\n", buffer);
            process_file(buffer, new_socket);
        }
        else if (bytes_received == 0)
        {
            printf("Client disconnected.\n");
        }
        else
        {
            perror("Receive failed");
        }

        close(new_socket);
    }

    close(server_fd);

    return 0;
}
