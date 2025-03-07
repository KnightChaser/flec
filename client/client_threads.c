// client/client_threads.c
#include "client_threads.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_PATH "/tmp/echo_socket"
#define BUFFER_SIZE 256

typedef struct {
    int start;
    int end;
} ThreadArgs;

void *send_request(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    int sock;
    struct sockaddr_un server_addr;
    char buffer[BUFFER_SIZE];

    // Create a Unix domain socket
    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Socket creation failed");
        return NULL;
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
        return NULL;
    }

    // Send requests in the range assigned to this thread
    for (int i = args->start; i < args->end; ++i) {
        snprintf(buffer, sizeof(buffer), "Request %d from thread", i);

        // Send and receive data
        if (send(sock, buffer, strlen(buffer), 0) == -1) {
            perror("Send failed");
            close(sock);
            return NULL;
        }

        ssize_t num_bytes = recv(sock, buffer, BUFFER_SIZE, 0);
        if (num_bytes == -1) {
            perror("Receive failed");
            close(sock);
            return NULL;
        }

        buffer[num_bytes] = '\0'; // Null-terminate received data
        // printf("Thread processed request %d\n", i);
    }

    // Clean up and close the socket
    close(sock);
    return NULL;
}
