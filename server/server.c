// server/server.c

#define _GNU_SOURCE
#include <fcntl.h>
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
#define BUFFER_SIZE 128
#define BACKLOG 100

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

// Function to handle the client connection with zero-copy using splice
void *client_handler(void *arg) {
    int client_sock = *(int *)arg;
    free(arg); // Free the client socket pointer allocated in the main function

    int pipe_fds[2]; // Pipe for zero-copy transfer
    if (pipe(pipe_fds) == -1) {
        perror("Pipe creation failed");
        close(client_sock);
        return NULL;
    }

    // Move data from the socket to the pipe
    while (true) {
        // Data is transferred from the client socket (client_sock) directly
        // into the pipe's write end (pipe_fds[1]) via zero-copy.
        ssize_t bytes_received = splice(client_sock, NULL, pipe_fds[1], NULL,
                                        BUFFER_SIZE, SPLICE_F_MORE);
        if (bytes_received == -1) {
            perror("splice failed");
            break;
        } else if (bytes_received == 0) {
            // No more data to read, client closed the connection
            break;
        }

        // The data is then transferred from the pipe's read end (pipe_fds[0])
        // to the client socket (client_sock) via zero-copy.
        if (splice(pipe_fds[0], NULL, client_sock, NULL, bytes_received,
                   SPLICE_F_MORE) == -1) {
            perror("splice failed");
            break;
        }
    }

    close(client_sock);
    close(pipe_fds[0]);
    close(pipe_fds[1]);
    return NULL;
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

    printf("Server listening on %s\n", SOCKET_PATH);

    while (true) {
        // Accept client connections.
        len = sizeof(struct sockaddr_un);
        int client_sock =
            accept(server_sock, (struct sockaddr *)&client_addr, &len);
        if (client_sock == -1) {
            perror("Accept failed");
            continue; // Continue accepting other clients
        }

        // Allocate memory for the client socket and pass it to the thread
        int *client_sock_ptr = malloc(sizeof(int));
        if (client_sock_ptr == NULL) {
            perror("Memory allocation failed");
            close(client_sock);
            continue;
        }
        *client_sock_ptr = client_sock;

        // Create a new thread to handle the client
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, client_handler,
                           (void *)client_sock_ptr) != 0) {
            perror("Thread creation failed");
            free(client_sock_ptr);
            close(client_sock);
        } else {
            pthread_detach(
                thread_id); // Detach the thread to allow auto cleanup
        }
    }

    // Clean up and close the server socket
    close(server_sock);
    unlink(SOCKET_PATH); // Remove the socket file

    return 0;
}
