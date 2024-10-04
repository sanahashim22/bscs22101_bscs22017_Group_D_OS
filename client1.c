#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     // For close()
#include <arpa/inet.h>  // For inet_addr() and htons()
#include <sys/socket.h> // For socket(), connect(), send(), recv()
#include <ctype.h>      // For isdigit()

#define PORT 8001
#define BUFFER_SIZE 2048

void encode_content(const char *input, char *output) {
    int input_length = strlen(input);
    int output_index = 0;

    for (int i = 0; i < input_length; i++) {
        char current_char = input[i];
        int count = 1;

        // Count consecutive occurrences of current_char
        while (i + 1 < input_length && input[i + 1] == current_char) {
            count++;
            i++;
        }

        // Append count and character to output
        output[output_index++] = count + '0'; // Convert count to char
        output[output_index++] = current_char;
    }

    output[output_index] = '\0'; // Null-terminate the string
}

void decode_content(const char *input, char *output) {
    int input_length = strlen(input);
    int output_index = 0;

    for (int i = 0; i < input_length; i++) {
        char current_char = input[i];

        if (isdigit(current_char)) {
            int count = current_char - '0'; // Convert char count to int

            // Append the next character 'count' times to output
            i++; // Move to the next character
            for (int j = 0; j < count; j++) {
                output[output_index++] = input[i];
            }
        } else {
            // Directly append non-numeric characters to output
            output[output_index++] = current_char;
        }
    }

    output[output_index] = '\0'; // Null-terminate the output
}

int main() {
    int sock;
    struct sockaddr_in server;
    char *message = "/home/sana/Desktop/bscs22101_bscs22017_Group_D_Lab4/command.txt";
    char server_response[BUFFER_SIZE] = {0};
    char file_content[BUFFER_SIZE] = {0};

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        return 1;
    }

    // Setup server address structure
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to server
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Connection failed");
        close(sock);
        return 1;
    }

    // Send the command file path to the server
    if (send(sock, message, strlen(message), 0) < 0) {
        perror("Send failed");
        close(sock);
        return 1;
    }

    printf("File path sent to server.\n");

    // Receive server response
    int bytes_received = recv(sock, server_response, sizeof(server_response) - 1, 0);
    if (bytes_received > 0) {
        server_response[bytes_received] = '\0'; // Null-terminate
        printf("Server response: %s\n", server_response); // Log the response

        // Response handling
        if (strstr(server_response, "Success: Ready to receive file.") != NULL) {
            FILE *file = fopen(message, "r");
            if (file == NULL) {
                printf("Could not open file: %s\n", message);
                close(sock);
                return 0;
            }

            char command[BUFFER_SIZE] = {0};
            char filepath[BUFFER_SIZE] = {0};
            char line[BUFFER_SIZE];

            // Simple parsing of the command file
            while (fgets(line, sizeof(line), file) != NULL) {
                if (strstr(line, "\"command\":") != NULL) {
                    sscanf(line, " \"command\": \"%[^\"]\"", command);
                } else if (strstr(line, "\"filepath\":") != NULL) {
                    sscanf(line, " \"filepath\": \"%[^\"]\"", filepath);
                }
            }
            fclose(file);

            FILE *file_to_send = fopen(filepath, "rb");
            if (file_to_send == NULL) {
                printf("Error: Could not open file %s for reading.\n", filepath);
                close(sock);
                return 0;
            }

            char file_chunk[BUFFER_SIZE];
            int bytes_read;

            // Read the file in chunks and send to the server
            while ((bytes_read = fread(file_chunk, 1, sizeof(file_chunk), file_to_send)) > 0) {
                char encoded_content[BUFFER_SIZE] = {0};
                encode_content(file_chunk, encoded_content);
                send(sock, encoded_content, strlen(encoded_content), 0);
            }

            fclose(file_to_send);
            printf("File '%s' sent successfully.\n", filepath);
        } else if (strstr(server_response, "File content: ") != NULL) {
            printf("%s", server_response); // Print received file content
            char content[BUFFER_SIZE] = {0};

            // Loop to receive file content in chunks
            while ((bytes_received = recv(sock, file_content, sizeof(file_content) - 1, 0)) > 0) {
                file_content[bytes_received] = '\0'; // Null-terminate
                decode_content(file_content, content);
                printf("%s", content); // Print decoded content
            }
            printf("\n");
        } else if (strstr(server_response, "File: ") != NULL) {
            printf("Files in directory received from server:\n%s\n", server_response);
        } else if (strstr(server_response, "Failure:") != NULL) {
            printf("Server response: %s\n", server_response);
        } else {
            printf("Unexpected response from server\n");
        }
    } else if (bytes_received == 0) {
        printf("Server closed the connection\n");
    } else {
        perror("Receive failed");
    }

    // Cleanup
    close(sock);
    return 0;
}
