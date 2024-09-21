#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     // For close()
#include <arpa/inet.h>  // For inet_addr() and htons()
#include <sys/socket.h> // For socket(), connect(), send(), recv()
#include <ctype.h>      // for isdigit()

#define PORT 8001
#define BUFFER_SIZE 2048

void encode_content(const char *input, char *output)
{
    int input_length = strlen(input);
    int output_index = 0;

    for (int i = 0; i < input_length; i++)
    {
        char current_char = input[i];
        int count = 1;

        // Count consecutive occurrences of current_char
        while (i + 1 < input_length && input[i + 1] == current_char)
        {
            count++;
            i++;
        }

        // Append count and character to output
        output[output_index++] = count + '0'; // Convert count to char
        output[output_index++] = current_char;
    }

    output[output_index] = '\0'; // Null-terminate the string
}

void decode_content(const char *input, char *output)
{
    int input_length = strlen(input);
    int output_index = 0;

    for (int i = 0; i < input_length; i++)
    {
        char current_char = input[i];

        if (isdigit(current_char))
        {
            int count = current_char - '0'; // Convert char count to int

            // Append the next character 'count' times to output
            i++; // Move to the next character
            for (int j = 0; j < count; j++)
            {
                output[output_index++] = input[i];
            }
        }
        else
        {
            // Directly append non-numeric characters to output
            output[output_index++] = current_char;
        }
    }

    // output[output_index] = '\0'; // Null-terminate the string
}

int main()
{
    int sock;
    struct sockaddr_in server;
    char *message = "/home/haris/Desktop/OS/Codes/bscs22101_bscs22017_Group_D_OS-main/command.txt";
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

        // Check if the response contains file content or a failure message
        if (strstr(server_response, "Success: Ready to receive file.") != NULL)
        {
            printf("server response: %s", server_response);
            FILE *file = fopen(message, "r");
            char line[BUFFER_SIZE];

            if (file == NULL)
            {
                printf("Could not open file: %s\n", message);
                return 0;
            }

            char filename[BUFFER_SIZE] = {0};
            char path[BUFFER_SIZE] = "/"; // Default to root for checking disk space
            char filepath[BUFFER_SIZE] = {0};

            // Simple parsing, assuming JSON format {"command": "upload", "filename": "example.txt"}
            while (fgets(line, sizeof(line), file) != NULL)
            {
                if (strstr(line, "\"filepath\":") != NULL)
                {
                    sscanf(line, " \"filepath\": \"%[^\"]\"", filepath);
                }
            }

            fclose(file);
            FILE *file_to_send = fopen(filepath, "rb");
            if (file_to_send == NULL)
            {
                printf("Error: Could not open file %s for reading.\n", filepath);
                return 0;
            }

            char file_content[BUFFER_SIZE] = {0};
            int bytes_read;

            // Read the file in chunks and send to the server
            while ((bytes_read = fread(file_content, 1, sizeof(file_content), file_to_send)) > 0)
            {
                char encoded_content[BUFFER_SIZE] = {0};
                encode_content(file_content, encoded_content);
                send(sock, encoded_content, strlen(encoded_content), 0);
                // send(sock, file_content, bytes_read, 0);
            }

            fclose(file_to_send);
            printf("File '%s' sent successfully.\n", filepath);
        }
        else if (strstr(server_response, "File content: ") != NULL)
        {
            // Print the file content received from the servers
            printf("%s", server_response);
            int bytes_received;

            char file_content[BUFFER_SIZE] = {0};
            char content[BUFFER_SIZE] = {0};

            // Loop to receive file content in chunks
            while ((bytes_received = recv(sock, file_content, sizeof(file_content), 0)) > 0)
            {
                decode_content(file_content, content);
                printf("%s", content);
            }
            printf("\n");
        }
        else if (strstr(server_response, "Last Modified: ") != NULL)
        {
            // Print the list of files with details (for "view" command)
            printf("Files in directory received from server:\n%s\n", server_response);
        }
        else if (strstr(server_response, "No files exist in the directory.") != NULL)
        {
            // Print the list of files with details (for "view" command)
            printf("No Client Data\n");
        }
        else if (strstr(server_response, "Failure:") != NULL)
        {
            // Print the failure message
            printf("Server response: %s\n", server_response);
        }
        else
        {
            printf("Failed to receive response from server\n");
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
