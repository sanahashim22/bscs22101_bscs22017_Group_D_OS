#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>

#define PORT 8001
#define BUFFER_SIZE 1024
#define FILE_PATH_BUFFER_SIZE 2048 // Increased buffer size for full file path

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

// Function to create a directory if it doesn't exist
void create_directory_if_not_exists(const char *path)
{
    struct stat st = {0};

    if (stat(path, &st) == -1)
    {
        if (mkdir(path, 0700) == 0)
        {
            printf("Directory created: %s\n", path);
        }
        else
        {
            perror("Failed to create directory");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        printf("Directory already exists: %s\n", path);
    }
}

// Function to process the file based on the command
// Function to process the file based on the command
void process_file(const char *file_path, int client_socket, const char *folder_path)
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
            char *last_slash = strrchr(filepath, '/');

            if (last_slash != NULL)
            {
                // Calculate the length of the directory path (excluding the file name)
                size_t length = last_slash - filepath;

                // Copy the directory part into dir_path
                strncpy(filepath, filepath, length);

                // Null-terminate the string
                filepath[length] = '\0';
                last_slash++;
                strncpy(filename, last_slash, strlen(last_slash));
            }
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

                // Construct the full file path inside the created folder
                char full_file_path[FILE_PATH_BUFFER_SIZE];
                if (snprintf(full_file_path, sizeof(full_file_path), "%s/%s", filepath, filename) >= sizeof(full_file_path))
                {
                    printf("Error: File path is too long.\n");
                    return;
                }

                // Write the received content to the file
                FILE *new_file = fopen(full_file_path, "w");
                if (new_file == NULL)
                {
                    printf("Could not create file: %s\n", full_file_path);
                    return;
                }

                fprintf(new_file, "%s", file_content);
                fclose(new_file);
                printf("File '%s' created successfully in directory: %s\n", filename, filepath);
            }
        }
        else
        {
            // Send failure message to client
            char failure_message[] = "Failure: Not enough disk space.";
            send(client_socket, failure_message, strlen(failure_message), 0);
            printf("Not enough disk space for file: %s\n", filename);
        }
        return;
    }

    else if (strcmp(command, "download") == 0)
    {
        char file_content[BUFFER_SIZE];
        FILE *file_to_send;
        int bytes_read;

        // Construct the full path to the file
        char full_file_path[FILE_PATH_BUFFER_SIZE];
        snprintf(full_file_path, sizeof(full_file_path), "%s/%s", folder_path, filename);

        // Open the file
        file_to_send = fopen(full_file_path, "r");
        if (file_to_send == NULL)
        {
            // File not found
            char failure_message[] = "Failure: File not found.";
            send(client_socket, failure_message, strlen(failure_message), 0);
            printf("File '%s' not found in directory '%s'.\n", filename, folder_path);
            return;
        }

        // Send the file content to the client
        while ((bytes_read = fread(file_content, 1, sizeof(file_content) - 1, file_to_send)) > 0)
        {
            file_content[bytes_read] = '\0';
            char success_message[] = "File content:";
            send(client_socket, file_content, strlen(success_message), 0);
        }

        fclose(file_to_send);
        printf("File '%s' sent to client from directory '%s'.\n", filename, folder_path);
        return;
    }
    else if (strcmp(command, "view") == 0)
    {
        DIR *dir;
        struct dirent *entry;
        struct stat file_stat;
        char file_path[FILE_PATH_BUFFER_SIZE];
        char message[BUFFER_SIZE * 2]; // Larger buffer for sending details

        // Open the directory
        if ((dir = opendir(folder_path)) != NULL)
        {
            // Loop through the files in the directory
            while ((entry = readdir(dir)) != NULL)
            {
                // Skip "." and ".."
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                {
                    continue;
                }

                // Construct the full path to the file
                snprintf(file_path, sizeof(file_path), "%s/%s", folder_path, entry->d_name);

                // Get file stats (size, modification time)
                if (stat(file_path, &file_stat) == 0)
                {

                    // Open the file and read its content
                    FILE *file = fopen(file_path, "r");
                    if (file != NULL)
                    {
                        char file_content[BUFFER_SIZE];
                        int bytes_read;

                        // Read the file in chunks and send the content to the client
                        while ((bytes_read = fread(file_content, 1, sizeof(file_content) - 1, file)) > 0)
                        {
                            file_content[bytes_read] = '\0';                  // Null-terminate the string
                            send(client_socket, file_content, bytes_read, 0); // Send the content
                        }

                        fclose(file); // Close the file after reading
                    }
                    else
                    {
                        perror("Error opening file for reading");
                    }
                }
                // Format the file details
                snprintf(message, sizeof(message), "\nFile:\n%s | Size: %lld bytes | Last Modified: %s\n",
                         entry->d_name, (long long)file_stat.st_size, ctime(&file_stat.st_mtime));
                send(client_socket, message, strlen(message), 0);
            }
            closedir(dir);
        }
        else
        {
            perror("Could not open directory");
        }
        return;
    }

    else
    {
        printf("Unknown command: %s\n", command);
        printf("Supported commands are: upload, download, view\n");
    }
    return;
}

int main()
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    // Define the folder path based on the port number
    char folder_path[BUFFER_SIZE];
    snprintf(folder_path, sizeof(folder_path), "/home/haris/Desktop/OS/Codes/%d", PORT);

    // Create the directory if it doesn't exist
    create_directory_if_not_exists(folder_path);

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
            process_file(buffer, new_socket, folder_path);
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