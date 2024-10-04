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
#include <pthread.h>
#include <ctype.h>

#define PORT 8001
#define MINI_BUFFER_SIZE 512
#define BUFFER_SIZE 1024
#define FILE_PATH_BUFFER_SIZE 2048

// Structure to pass arguments to the thread function
struct client_info {
    int client_socket;
    char folder_path[FILE_PATH_BUFFER_SIZE];
};

// Mutex for synchronizing file operations
pthread_mutex_t mutex;

// Function to check available disk space in bytes
unsigned long long get_free_space(const char *path) {
    struct statvfs stat;

    if (statvfs(path, &stat) != 0) {
        // Error getting filesystem statistics
        return 0;
    }

    // Calculate the available space in bytes
    return stat.f_bsize * stat.f_bavail;
}

// Function to create a directory if it doesn't exist
void create_directory_if_not_exists(const char *path) {
    struct stat st = {0};

    if (stat(path, &st) == -1) {
        if (mkdir(path, 0700) == 0) {
            printf("Directory created: %s\n", path);
        } else {
            perror("Failed to create directory");
            exit(EXIT_FAILURE);
        }
    } else {
        printf("Directory already exists: %s\n", path);
    }
}

// Function to process the file based on the command
void process_file(const char *file_path, int client_socket, const char *folder_path) {
    FILE *file = fopen(file_path, "r");
    char line[BUFFER_SIZE];

    if (file == NULL) {
        printf("Could not open file: %s\n", file_path);
        return;
    }

    char command[BUFFER_SIZE] = {0};
    char filename[BUFFER_SIZE] = {0};
    char path[BUFFER_SIZE] = "/";
    char filepath[BUFFER_SIZE] = {0};
    char client_dir[FILE_PATH_BUFFER_SIZE * 3] = {0};
    char id[MINI_BUFFER_SIZE] = {0};

    // Simple parsing, assuming JSON format {"command": "upload", "filename": "example.txt"}
    while (fgets(line, sizeof(line), file) != NULL) {
        if (strstr(line, "\"command\":") != NULL) {
            sscanf(line, " \"command\": \"%[^\"]\"", command);
        } else if (strstr(line, "\"ID\":") != NULL) {
            sscanf(line, " \"ID\": \"%[^\"]\"", id);
        } else if (strstr(line, "\"filename\":") != NULL) {
            sscanf(line, " \"filename\": \"%[^\"]\"", filename);
        } else if (strstr(line, "\"filepath\":") != NULL) {
            sscanf(line, " \"filepath\": \"%[^\"]\"", filepath);
            char *last_slash = strrchr(filepath, '/');

            if (last_slash != NULL) {
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
    snprintf(client_dir, sizeof(client_dir), "/home/sana/Desktop/bscs22101_bscs22017_Group_D_Lab4/%s", id);

    if (strcmp(command, "upload") == 0) {
        create_directory_if_not_exists(client_dir);
        unsigned long long free_space = get_free_space(client_dir);
        printf("Free space on path %s: %llu bytes\n", client_dir, free_space);

        if (free_space > 10000) { // Check if there is more than 10KB free space
            char success_message[] = "Success: Ready to receive file.";
            send(client_socket, success_message, strlen(success_message), 0);

            if (snprintf(client_dir + strlen(client_dir), sizeof(client_dir) - strlen(client_dir), "/%s", filename) >= (sizeof(client_dir) - strlen(client_dir))) {
                printf("Error: client_dir path too long.\n");
                return;
            }

            // Open the new file where content will be written
            FILE *new_file = fopen(client_dir, "wb");
            if (new_file == NULL) {
                printf("Could not create file: %s\n", client_dir);
                return;
            }

            // Receive file content in chunks from the client
            char file_content[BUFFER_SIZE] = {0};
            int bytes_received;

            // Lock mutex before receiving file content
            pthread_mutex_lock(&mutex);

            // Loop to receive file content in chunks
            while ((bytes_received = recv(client_socket, file_content, sizeof(file_content), 0)) > 0) {
                fwrite(file_content, 1, bytes_received, new_file);
            }

            // Unlock mutex after file operations
            pthread_mutex_unlock(&mutex);

            // Close the file after all data is received
            fclose(new_file);
            printf("File '%s' uploaded successfully to directory: %s\n", filename, client_dir);

        } else {
            char failure_message[] = "Failure: Not enough disk space.";
            send(client_socket, failure_message, strlen(failure_message), 0);
            printf("Not enough disk space for file: %s\n", filename);
        }

        return;
    } else if (strcmp(command, "download") == 0) {
        char file_content[BUFFER_SIZE];
        FILE *file_to_send;
        int bytes_read;

        // Construct the full path to the file
        printf("client dir: %s\n", client_dir);
        char full_file_path[FILE_PATH_BUFFER_SIZE] = "/home/sana/Desktop/bscs22101_bscs22017_Group_D_Lab4";
        snprintf(client_dir, sizeof(client_dir), "%s/%s/%s", full_file_path, id, filename);

        // Open the file
        file_to_send = fopen(client_dir, "rb"); // Use "rb" for reading binary files
        if (file_to_send == NULL) {
            char failure_message[] = "Failure: File not found.";
            send(client_socket, failure_message, strlen(failure_message), 0);
            printf("File '%s' not found in directory '%s'.\n", filename, client_dir);
            return;
        }

        char success_message[BUFFER_SIZE] = "File content: ";
        send(client_socket, success_message, strlen(success_message), 0);

        // Lock mutex before sending file content
        pthread_mutex_lock(&mutex);

        // Send the file content to the client
        while ((bytes_read = fread(file_content, 1, sizeof(file_content), file_to_send)) > 0) {
            send(client_socket, file_content, bytes_read, 0);
        }

        // Unlock mutex after file operations
        pthread_mutex_unlock(&mutex);

        fclose(file_to_send);
        printf("File '%s' sent to client from directory '%s'.\n", filename, client_dir);
        return;
    } else if (strcmp(command, "view") == 0) {
        DIR *dir;
        struct dirent *entry;
        struct stat file_stat;
        char file_path[FILE_PATH_BUFFER_SIZE * 4];
        char message[BUFFER_SIZE * 2]; // Larger buffer for sending details

        // Open the directory
        if ((dir = opendir(client_dir)) != NULL) {
            // Lock mutex before accessing the directory
            pthread_mutex_lock(&mutex);

            // Loop through the files in the directory
            while ((entry = readdir(dir)) != NULL) {
                // Skip "." and ".."
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                    continue;
                }

                // Construct the full path to the file
                snprintf(file_path, sizeof(file_path), "%s/%s", client_dir, entry->d_name);

                // Get file stats (size, modification time)
                if (stat(file_path, &file_stat) == 0) {
                    // Format the file details
                    snprintf(message, sizeof(message), "File: %s | Size: %ld bytes | Last modified: %s\n", entry->d_name, (long)file_stat.st_size, ctime(&file_stat.st_mtime));
                    send(client_socket, message, strlen(message), 0);
                }
            }

            // Unlock mutex after accessing the directory
            pthread_mutex_unlock(&mutex);

            closedir(dir); // Close the directory
        } else {
            perror("Error opening directory for reading");
        }
        return;
    }
}

// Thread function to handle client requests
void *handle_client(void *arg) {
    struct client_info *info = (struct client_info *)arg;
    int client_socket = info->client_socket;
    char *folder_path = info->folder_path;

    // Buffer for receiving commands from the client
    char buffer[BUFFER_SIZE];

    // Receive the command from the client
    int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytes_received <= 0) {
        printf("Client disconnected or error receiving data.\n");
        close(client_socket);
        free(info);
        return NULL;
    }

    buffer[bytes_received] = '\0'; // Null-terminate the received data

    // Process the file based on the received command
    process_file(buffer, client_socket, folder_path);

    // Close the client socket
    close(client_socket);
    free(info);
    return NULL;
}

// Main function
int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // Initialize mutex
    pthread_mutex_init(&mutex, NULL);

    // Create server socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        return EXIT_FAILURE;
    }

    // Set server address and port
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind the socket
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Binding failed");
        close(server_socket);
        return EXIT_FAILURE;
    }

    // Start listening for incoming connections
    if (listen(server_socket, 5) == -1) {
        perror("Listening failed");
        close(server_socket);
        return EXIT_FAILURE;
    }

    printf("Server is listening on port %d...\n", PORT);

    while (1) {
        // Accept a new client connection
        if ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len)) == -1) {
            perror("Accepting connection failed");
            continue;
        }

        // Allocate memory for client info
        struct client_info *info = malloc(sizeof(struct client_info));
        info->client_socket = client_socket;
        strncpy(info->folder_path, "/home/sana/Desktop/bscs22101_bscs22017_Group_D_Lab4", FILE_PATH_BUFFER_SIZE);

        // Create a new thread to handle the client
        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, info) != 0) {
            perror("Thread creation failed");
            free(info);
            close(client_socket);
        }
    }

    // Cleanup
    close(server_socket);
    pthread_mutex_destroy(&mutex);
    return EXIT_SUCCESS;
}
