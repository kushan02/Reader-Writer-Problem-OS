#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "CannotResolve"
#define NUM_READERS 5
#define NUM_WRITERS 5

void *readerMain(void *threadArgument);

void *writerMain(void *threadArgument);

/* GLOBAL SHARED DATA ====================================================== */
unsigned int shared_value = 0;

pthread_mutex_t gSharedMemoryLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t gReadPhase = PTHREAD_COND_INITIALIZER;
pthread_cond_t gWritePhase = PTHREAD_COND_INITIALIZER;

int gWaitingReaders = 0, gReaders = 0;

int active_readers = 0;


int main(int argc, char **argv) {


    int readerNum[NUM_READERS];
    int writerNum[NUM_WRITERS];

    pthread_t readerThreadIDs[NUM_READERS];
    pthread_t writerThreadIDs[NUM_WRITERS];

    // Seed the random number generator
    srandom((unsigned int) time(NULL));


    // Start the readers
    for (int j = 0; j < NUM_READERS; j++) {
        readerNum[j] = j;
        pthread_create(&readerThreadIDs[j], NULL, readerMain, &readerNum[j]);
    }

    // Start the writers
    for (int k = 0; k < NUM_WRITERS; k++) {
        writerNum[k] = k;
        pthread_create(&writerThreadIDs[k], NULL, writerMain, &writerNum[k]);
    }

    // Wait on readers to finish
    for (int i = 0; i < NUM_READERS; i++) {
        pthread_join(readerThreadIDs[i], NULL);
    }

    // Wait on writers to finish
    for (int i = 0; i < NUM_WRITERS; i++) {
        pthread_join(writerThreadIDs[i], NULL);
    }

    return 0;
}

void *readerMain(void *threadArgument) {

    int id = *((int *) threadArgument);
    //int i = 0;
    static int numReaders = 0;

    active_readers++;

    //  for(i = 0; i < NUM_READS; i++) {

    // Wait so that reads and writes do not all happen at once
    int random_number = random();
    int wait_time = random_number % 10;
    usleep(1000 * wait_time);

    // Enter critical section
    pthread_mutex_lock(&gSharedMemoryLock);
    gWaitingReaders++;
    while (gReaders == -1) {
        pthread_cond_wait(&gReadPhase, &gSharedMemoryLock);
    }
    gWaitingReaders--;
    numReaders = ++gReaders;
    active_readers--;
    pthread_mutex_unlock(&gSharedMemoryLock);

    // Read data
    // fprintf(stdout, "[r%d] reading %u  [readers: %2d]\n", id, shared_value, numReaders);

    // fprintf(stdout, "[r%d] reading %u  [readers: %2d]\n", id, shared_value, active_readers);
    fprintf(stdout, "r%-1d %-3u %-3d\n", id, shared_value, active_readers);


    // Exit critical section
    pthread_mutex_lock(&gSharedMemoryLock);
    gReaders--;
    if (gReaders == 0) {
        pthread_cond_signal(&gWritePhase);
    }
    pthread_mutex_unlock(&gSharedMemoryLock);
    // }

    pthread_exit(0);
}

void *writerMain(void *threadArgument) {

    int id = *((int *) threadArgument);
    // int i = 0;
    int numReaders = 0;

    //  for(i = 0; i < NUM_WRITES; i++) {
    // Wait so that reads and writes do not all happen at once
    //usleep(1000 * (random() % NUM_READERS + NUM_WRITERS));
    int random_number = random();
    int wait_time = random_number % 10;
    usleep(1000 * wait_time);


    // Enter critical section
    pthread_mutex_lock(&gSharedMemoryLock);
    while (gReaders != 0) {
        pthread_cond_wait(&gWritePhase, &gSharedMemoryLock);
    }
    gReaders = -1;
    numReaders = gReaders;
    pthread_mutex_unlock(&gSharedMemoryLock);

    // Set Custom Value of shared_value
    shared_value = random() % 999;

    // Write data
    // fprintf(stdout, "[w%d] writing %u* [readers: %2d]\n", id, ++shared_value, numReaders);
    //fprintf(stdout, "[w%d] writing %u* [readers: %2d]\n", id, ++shared_value, active_readers);
    fprintf(stdout, "w%-1d %-3u %-3d\n", id, shared_value, active_readers);


    // Exit critical section
    pthread_mutex_lock(&gSharedMemoryLock);
    gReaders = 0;
    if (gWaitingReaders > 0) {
        pthread_cond_broadcast(&gReadPhase);
    } else {
        pthread_cond_signal(&gWritePhase);
    }
    pthread_mutex_unlock(&gSharedMemoryLock);
    // }

    pthread_exit(0);
}

#pragma clang diagnostic pop
