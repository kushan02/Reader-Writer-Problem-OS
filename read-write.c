#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_READERS 5
#define MAX_WRITERS 5

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

    int reader_array[MAX_READERS];
    int writer_array[MAX_WRITERS];

    pthread_t readerThreadIDs[MAX_READERS];
    pthread_t writerThreadIDs[MAX_WRITERS];

    srandom((unsigned int) time(NULL));

    for (int j = 0; j < MAX_READERS; j++) {
        reader_array[j] = j;
        pthread_create(&readerThreadIDs[j], NULL, readerMain, &reader_array[j]);
    }

    for (int k = 0; k < MAX_WRITERS; k++) {
        writer_array[k] = k;
        pthread_create(&writerThreadIDs[k], NULL, writerMain, &writer_array[k]);
    }

    for (int i = 0; i < MAX_READERS; i++) {
        pthread_join(readerThreadIDs[i], NULL);
    }

    for (int i = 0; i < MAX_WRITERS; i++) {
        pthread_join(writerThreadIDs[i], NULL);
    }

    return 0;
}

void *readerMain(void *threadArgument) {

    int id = *((int *) threadArgument);

    active_readers++;

    // Wait so that reads and writes do not all happen at once
    int random_number = random();
    int wait_time = random_number % 10;
    usleep(1000 * wait_time);

    pthread_mutex_lock(&gSharedMemoryLock);
    gWaitingReaders++;
    while (gReaders == -1) {
        pthread_cond_wait(&gReadPhase, &gSharedMemoryLock);
    }
    gWaitingReaders--;
    ++gReaders;
    active_readers--;
    pthread_mutex_unlock(&gSharedMemoryLock);

    // Read data
    fprintf(stdout, "r%-1d %-3u %-3d\n", id, shared_value, active_readers);

    pthread_mutex_lock(&gSharedMemoryLock);
    gReaders--;
    if (gReaders == 0) {
        pthread_cond_signal(&gWritePhase);
    }
    pthread_mutex_unlock(&gSharedMemoryLock);
    pthread_exit(0);
}

void *writerMain(void *threadArgument) {

    int id = *((int *) threadArgument);

    // Wait so that reads and writes do not all happen at once
    int random_number = random();
    int wait_time = random_number % 10;
    usleep(1000 * wait_time);

    pthread_mutex_lock(&gSharedMemoryLock);
    while (gReaders != 0) {
        pthread_cond_wait(&gWritePhase, &gSharedMemoryLock);
    }
    gReaders = -1;
    pthread_mutex_unlock(&gSharedMemoryLock);

    // Set custom value of shared_value
    shared_value = random() % 999;

    // Write data
    fprintf(stdout, "w%-1d %-3u %-3d\n", id, shared_value, active_readers);

    pthread_mutex_lock(&gSharedMemoryLock);
    gReaders = 0;
    if (gWaitingReaders > 0) {
        pthread_cond_broadcast(&gReadPhase);
    } else {
        pthread_cond_signal(&gWritePhase);
    }
    pthread_mutex_unlock(&gSharedMemoryLock);

    pthread_exit(0);
}
