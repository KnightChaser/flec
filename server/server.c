// server/server.c
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_PATH "/tmp/echo_socket"
#define BUFFER_SIZE 256
#define BACKLOG 100 // Increase the backlog for pending connections

int main(int argc, char *argv[]) {
    int server_sock, client_sock;
    socklen_t len;
    struct sockaddr_un server_addr, client_addr;
    char buffer[BUFFER_SIZE];

    // Create a Unix domain socket.
    server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up the server address structure
    memset(&server_addr, 0, sizeof(struct sockaddr_un));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH,
            sizeof(server_addr.sun_path) - 1);

    // Bind the socket to the path
    int optval = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &optval,
                   sizeof(optval)) == -1) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    if (bind(server_sock, (struct sockaddr *)&server_addr,
             sizeof(struct sockaddr_un)) == -1) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections (allow a larger backlog)
    if (listen(server_sock, BACKLOG) == -1) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on %s\n", SOCKET_PATH);

    while (true) {
        // Accept client connections.
        len = sizeof(struct sockaddr_un);
        client_sock =
            accept(server_sock, (struct sockaddr *)&client_addr, &len);
        if (client_sock == -1) {
            perror("Accept failed");
            continue; // Continue accepting other clients
        }

        // printf("Client connected\n");

        // Process each request sequentially
        while (true) {
            ssize_t bytes_received = recv(client_sock, buffer, BUFFER_SIZE, 0);
            if (bytes_received == -1) {
                perror("Receive failed");
                break;
            } else if (bytes_received == 0) {
                // printf("Client disconnected\n");
                break; // Exit the loop when the client disconnects
            }

            buffer[bytes_received] = '\0';
            // printf("Received: %s\n", buffer);

            // Send the received data back to the client
            if (send(client_sock, buffer, bytes_received, 0) == -1) {
                perror("Send failed");
                break;
            }
        }

        // Clean up and close the client socket
        close(client_sock);
    }

    // Clean up and close the server socket
    close(server_sock);
    unlink(SOCKET_PATH); // Remove the socket file

    return 0;
}
