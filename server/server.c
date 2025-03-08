// server/server.c
#include "thread_pool.h"
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_PATH "/tmp/echo_socket"
#define BUFFER_SIZE 256
#define BACKLOG 100
#define THREAD_COUNT 100
#define QUEUE_SIZE 100

int server_sock = -1; // Global variable for the server socket

// Signal handler for graceful shutdown
void handle_shutdown(int sig) {
    if (server_sock != -1) {
        // Close the server socket
        close(server_sock);
        unlink(SOCKET_PATH); // Remove the socket file
        printf("\nServer shut down gracefully.\n");
    }
    exit(0);
}

int main(int argc, char *argv[]) {
    struct sockaddr_un server_addr, client_addr;
    socklen_t len;

    // Set up the signal handler for SIGINT (Ctrl+C)
    signal(SIGINT, handle_shutdown);

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

    // Create the thread pool to handle the client's requests
    thread_pool_t pool;
    if (thread_pool_create(&pool, THREAD_COUNT, QUEUE_SIZE) == -1) {
        perror("Thread pool creation failed");
        close(server_sock);
        unlink(SOCKET_PATH);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on %s\n", SOCKET_PATH);

    while (true) {
        len = sizeof(struct sockaddr_un);
        int *client_sock = malloc(sizeof(int));

        *client_sock =
            accept(server_sock, (struct sockaddr *)&client_addr, &len);
        if (*client_sock == -1) {
            perror("Accept failed");
            free(client_sock);
            continue;
        }

        // Add the client socket to the task queue(thread pool)
        if (thread_pool_add_task(&pool, client_sock) != 0) {
            perror("Failed to add client socket to the task queue");
            free(client_sock);
        }
    }

    // Clean up and close the server socket
    thread_pool_destroy(&pool);
    close(server_sock);
    unlink(SOCKET_PATH); // Remove the socket file

    return 0;
}
