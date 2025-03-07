// client/client.c
#include "client_threads.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM_REQUESTS 3000000
#define NUM_THREADS 100
#define REQUESTS_PER_THREAD (NUM_REQUESTS / NUM_THREADS)

typedef struct {
    int start;
    int end;
} ThreadArgs;

int main(int argc, char *argv[]) {
    printf("Client started...\n");

    // Log the start time before the pthread loop
    clock_t start_time = clock();

    // Spawn threads to send requests
    pthread_t client_threads[NUM_THREADS];
    ThreadArgs args[NUM_THREADS];

    // Divide the work (requests) among threads
    for (int i = 0; i < NUM_THREADS; ++i) {
        args[i].start = i * REQUESTS_PER_THREAD;
        args[i].end = (i + 1) * REQUESTS_PER_THREAD;
        if (pthread_create(&client_threads[i], NULL, send_request,
                           (void *)&args[i]) != 0) {
            perror("Failed to create client thread");
            return EXIT_FAILURE;
        }
    }

    // Wait for all threads to finish
    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_join(client_threads[i], NULL);
    }

    // Log the end time after the pthread loop
    clock_t end_time = clock();

    // Obtain the statistics
    double time_taken = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    double throughput = NUM_REQUESTS / time_taken;

    // Output the time and throughput
    printf("Total requests: %d requests\n", NUM_REQUESTS);
    printf("Time taken: %.2f seconds\n", time_taken);
    printf("Throughput: %.2f requests/sec\n", throughput);

    printf("Client finished all requests.\n");
    return 0;
}
