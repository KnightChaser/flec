// client/client.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_PATH "/tmp/echo_socket"
#define BUFFER_SIZE 256

int main() {
    int sock;
    struct sockaddr_un server_addr;
    char buffer[BUFFER_SIZE];

    // Create a Unix domain socket
    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up the server address structure
    memset(&server_addr, 0, sizeof(struct sockaddr_un));
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, SOCKET_PATH);

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&server_addr,
                sizeof(struct sockaddr_un)) == -1) {
        perror("Connect failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Connected to server!\n");

    // Send and receive data in a loop
    while (1) {
        printf("Enter message to send: ");
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL)
            break;

        // Remove newline character at the end of input
        buffer[strcspn(buffer, "\n")] = '\0';

        // Send message to the server
        if (send(sock, buffer, strlen(buffer), 0) == -1) {
            perror("Send failed");
            break;
        }

        // Receive echo from server
        ssize_t num_bytes = recv(sock, buffer, BUFFER_SIZE, 0);
        if (num_bytes == -1) {
            perror("Receive failed");
            break;
        }
        buffer[num_bytes] = '\0'; // Null-terminate received data
        printf("Echo from server: %s\n", buffer);
    }

    // Clean up and close the socket
    close(sock);
}
