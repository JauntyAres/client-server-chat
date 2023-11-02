#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define FRAME_DATASIZE 32
#define FRAME_PACKETTSIZE (FRAME_DATASIZE + 3) * 8

void handleClient(int client_socket) {
    char buffer[FRAME_PACKETTSIZE + 1];  // +1 for null terminator
    ssize_t bytesRead;

    // Read data from the client
    while ((bytesRead = read(client_socket, buffer, FRAME_PACKETTSIZE)) > 0) {
        buffer[bytesRead] = '\0';  // Null-terminate the received data

        // Process the received data (execute binary for each frame)
        for (int i = 0; i < bytesRead; i += FRAME_PACKETTSIZE) {
            // Extract the current frame
            char frame[FRAME_PACKETTSIZE + 1];
            strncpy(frame, buffer + i, FRAME_PACKETTSIZE);
            frame[FRAME_PACKETTSIZE] = '\0';  // Null-terminate the frame

            // Execute the binary with the frame as an argument
            execl("./application", "./application", "0", frame, client_socket, NULL);
            // Note: If execl is successful, the code after it won't execute.

            // Handle execl error (if it reaches here, something went wrong)
            perror("execl failed");
            exit(EXIT_FAILURE);
        }

        // Send a reply (dummy response)
        const char *reply = "Reply from server: Frames received and processed successfully\n";
        write(client_socket, reply, strlen(reply));
    }

    if (bytesRead < 0) {
        perror("read");
    }

    close(client_socket);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    int port = atoi(argv[1]);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options to reuse the address
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Forcefully attaching socket to the specified port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // Handle the client in a separate function
        handleClient(new_socket);
    }

    close(server_fd);
    return 0;
}
