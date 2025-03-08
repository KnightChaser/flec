// server/thread_pool.h
#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>

typedef struct {
    int *client_sock;
} task_t;

typedef struct {
    pthread_t *threads;
    task_t *task_queue;
    int queue_size;
    int queue_front;
    int queue_back;
    int queue_count;
    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_cond;
    int thread_count;
    int shutdown;
} thread_pool_t;

// Function prototypes
int thread_pool_create(thread_pool_t *pool, int thread_count, int queue_size);
int thread_pool_add_task(thread_pool_t *pool, int *client_sock);
void *thread_pool_worker(void *arg);
void thread_pool_destroy(thread_pool_t *pool);

#endif // THREAD_POOL_H
