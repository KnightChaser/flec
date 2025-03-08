// server/thread_pool.c
#include "thread_pool.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int thread_pool_create(thread_pool_t *pool, int thread_count, int queue_size) {
    pool->thread_count = thread_count;
    pool->queue_size = queue_size;
    pool->queue_front = 0;
    pool->queue_back = 0;
    pool->queue_count = 0;
    pool->shutdown = 0;

    pool->task_queue = malloc(sizeof(task_t) * queue_size);
    pool->threads = malloc(sizeof(pthread_t) * thread_count);

    if (!pool->task_queue || !pool->threads) {
        perror("Memory allocation failed");
        return -1;
    }

    pthread_mutex_init(&pool->queue_mutex, NULL);
    pthread_cond_init(&pool->queue_cond, NULL);

    // Create the worker threads
    for (int i = 0; i < thread_count; i++) {
        if (pthread_create(&pool->threads[i], NULL, thread_pool_worker,
                           (void *)pool) != 0) {
            perror("Failed to create worker thread");
            return -1;
        }
    }

    return 0;
}

int thread_pool_add_task(thread_pool_t *pool, int *client_sock) {
    pthread_mutex_lock(&pool->queue_mutex);

    // Wait if the queue is full
    while (pool->queue_count == pool->queue_size) {
        pthread_cond_wait(&pool->queue_cond, &pool->queue_mutex);
    }

    // Add the task to the queu
    pool->task_queue[pool->queue_back].client_sock = client_sock;
    pool->queue_back = (pool->queue_back + 1) % pool->queue_size;
    pool->queue_count++;

    // Signal a worker thread to start the job
    pthread_cond_signal(&pool->queue_cond);

    pthread_mutex_unlock(&pool->queue_mutex);

    return 0;
}

void *thread_pool_worker(void *arg) {
    thread_pool_t *pool = (thread_pool_t *)arg;
    task_t task;

    while (1) {
        pthread_mutex_lock(&pool->queue_mutex);

        // Wait for tasks if the queue is empty
        while (pool->queue_count == 0 && !pool->shutdown) {
            pthread_cond_wait(&pool->queue_cond, &pool->queue_mutex);
        }

        if (pool->shutdown) {
            pthread_mutex_unlock(&pool->queue_mutex);
            break;
        }

        // Retrieve the task from the queue
        task = pool->task_queue[pool->queue_front];
        pool->queue_front = (pool->queue_front + 1) % pool->queue_size;
        pool->queue_count--;

        // Signal that there is space in the queue for new tasks
        pthread_cond_signal(&pool->queue_cond);
        pthread_mutex_unlock(&pool->queue_mutex);

        // Process multiple requests on the same connection.
        // The loop will continue until the client disconnects or an error
        // occurs.
        char buffer[256];
        ssize_t bytes_received;
        while ((bytes_received =
                    recv(*(task.client_sock), buffer, sizeof(buffer), 0)) > 0) {
            if (send(*(task.client_sock), buffer, bytes_received, 0) == -1) {
                perror("Send failed");
                break;
            }
        }

        // Close the client connection and free the allocated memory
        close(*(task.client_sock));
        free(task.client_sock);
    }

    pthread_exit(NULL);
}
