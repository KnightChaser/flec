#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_PATH "/tmp/echo_socket"
#define MAX_EVENTS 10
#define BUFFER_SIZE 256
#define NUM_THREADS 25

int server_sock = -1; // Global variable for the server socket
pthread_mutex_t server_sock_mutex = PTHREAD_MUTEX_INITIALIZER;

// Function to set the socket to non-blocking mode
int set_non_blocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl failed");
        return -1;
    }

    flags |= O_NONBLOCK;
    if (fcntl(sockfd, F_SETFL, flags) == -1) {
        perror("fcntl failed");
        return -1;
    }
    return 0;
}

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

// Worker thread function to handle each client connection
void *client_handler(void *arg) {
    int client_sock = *((int *)arg);
    free(arg);
    char buffer[BUFFER_SIZE];

    while (1) {
        ssize_t bytes_received = recv(client_sock, buffer, sizeof(buffer), 0);
        if (bytes_received == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No data available right now, just continue
                continue;
            } else {
                perror("Receive failed");
                break;
            }
        } else if (bytes_received == 0) {
            // Client disconnected
            break;
        }

        buffer[bytes_received] = '\0'; // Null-terminate the received data

        // Echo the received data back to the client
        if (send(client_sock, buffer, bytes_received, 0) == -1) {
            perror("Send failed");
            break;
        }
    }

    // Close the client socket
    close(client_sock);
    return NULL;
}

int main() {
    struct sockaddr_un server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    struct epoll_event event, events[MAX_EVENTS];
    int epoll_fd, nfds;

    // Register signal handler for graceful shutdown
    signal(SIGINT, handle_shutdown);

    // Create server socket
    server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, SOCKET_PATH);

    if (bind(server_sock, (struct sockaddr *)&server_addr,
             sizeof(server_addr)) == -1) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, 10) == -1) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    // Set up epoll
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1 failed");
        exit(EXIT_FAILURE);
    }

    // Set server socket to non-blocking mode
    if (set_non_blocking(server_sock) == -1) {
        exit(EXIT_FAILURE);
    }

    // Add server socket to epoll
    event.events = EPOLLIN;
    event.data.fd = server_sock;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_sock, &event) == -1) {
        perror("epoll_ctl failed");
        exit(EXIT_FAILURE);
    }

    // Event loop
    while (1) {
        nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait failed");
            exit(EXIT_FAILURE);
        }

        // Loop through events
        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == server_sock) {
                // Accept a new connection
                socklen_t client_len = sizeof(client_addr);
                int client_sock = accept(
                    server_sock, (struct sockaddr *)&client_addr, &client_len);
                if (client_sock == -1) {
                    perror("Accept failed");
                    continue;
                }

                // Set client socket to non-blocking mode
                if (set_non_blocking(client_sock) == -1) {
                    continue;
                }

                // Create a new thread for the client
                pthread_t thread_id;
                int *client_sock_ptr = malloc(sizeof(int));
                *client_sock_ptr = client_sock;
                if (pthread_create(&thread_id, NULL, client_handler,
                                   client_sock_ptr) != 0) {
                    perror("Thread creation failed");
                    free(client_sock_ptr);
                    close(client_sock);
                    continue;
                }

                // Detach the thread to allow for automatic cleanup
                pthread_detach(thread_id);
            }
        }
    }

    // Clean up
    close(server_sock);
    unlink(SOCKET_PATH);
    return 0;
}
