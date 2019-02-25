#ifndef SHARED_H
#define SHARED_H

/* Needed for pthread_* */
#include <pthread.h>

/* Needed for sleep() */
#include <unistd.h>

/* For fwrite etc. */
#include <stdio.h>

/* For malloc() etc. */
#include <stdlib.h>

/* For false etc. */
#include <stdbool.h>

/* Name of the file containing data to be read from the filesystem. */
#define SHARED_FILE_NAME "shared_data"

/* Name of the shared memory region. */
#define SHARED_FILE_BUFFER_NAME "data_buffer"

/* Simulator output filename. */
#define SHARED_FILE_SIM_OUT_NAME "sim_out"

/* The number of entries in the shared memory buffer. */
#define SHM_BUFFER_SIZE (20)

/* The number of bytes in the shared memory buffer. */
#define SHM_BUFFER_SIZE_BYTES (SHM_BUFFER_SIZE * sizeof(int))

/* The number of elements in the shared_data file. */
#define NUM_ELEMENTS_DATA_FILE (100)

/*
 * Struct to store the command-line configuration for the program.
 */
typedef struct ProgramConfig
{
    /* How many readers to create. */
    int readerCount;

    /* How many writers to create. */
    int writerCount;

    /* How long each reader should sleep for once it is done reading what it can. */
    int readerSleepTime;

    /* How long each writer should sleep for once it is done writing an element to the buffer. */
    int writerSleepTime;

} ProgramConfig;

/*
 * Container for reader/writer configuration.
 *
 * Provides information required by the reader/writer algorithms.
 *
 * Encapsulates the program data for use by reader/writers.
 */
typedef struct RWConfig
{
    /* The shared memory. Since the threading component of the solution does not need shared memory,
     * (because memory is shared betweeh threads), this is just an array shared between threads.
     * The size of this array will be SHM_BUFFER_SIZE. */
    int *data;

    /* The index we are currently writing to. Writers use this to cooperate in writing to the data buffer. */
    int idxWrite;

    /* The program's configuration. This is information that is provided by the user on the command line. */
    ProgramConfig *pConfig;

    /* Store the number of readers so that we know when to enable writers to write. */
    int activeReaders;

    /* Per specification: the number of writes performed. */
    int writes;

    /* Mutex for modifying the activeReaders variable. */
    pthread_mutex_t rcMutex;

    /* Conditional variables to ensure writers and readers can wake the others if:
     * 1) readers attempt to read without any full buffers,
     * 2) writers attempt to write without any empty buffers
     */
    pthread_cond_t emptyCond;
    pthread_cond_t fullCond;

    /* Mutex lock to enable/disable writers from entering their critical sections. */
    /*pthread_mutex_t writeMutex;*/
    pthread_mutex_t writeMutex;

    /* Mutex lock to enable/disable readers/writers from changing the pendingReads value. */
    pthread_mutex_t rpMutex;
    
    /* Mutex lock to enable/disable writers from changing the write count value. */
    pthread_mutex_t wcMutex;

    /* Mutex lock to enable/disable writers from changing the idxWrite value. */
    pthread_mutex_t idxWriteMutex;

    pthread_mutex_t rwMutex;

    /* The number of pending reads for each particular shared memory slot. This should point to
     * an array of size S, where S is the number of shared memory slots. */
    int *pendingReads;

    /* sim_out file reference. */
    FILE *fPtrSimOut;

    /* shared_data file reference. */
    FILE *fPtrSharedData;
} RWConfig;

RWConfig *createRWConfig(ProgramConfig *);
void freeRWConfig(RWConfig *);
int *ret(int);
int *createDefaultValueArray(int, int);
void simWriteFinish(FILE *fPtr, char *type, char *action, char *dest, int id, int val);

#endif /* ifndef SHARED_H */
