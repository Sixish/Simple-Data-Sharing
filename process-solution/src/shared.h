#ifndef SHARED_H
#define SHARED_H

#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

/* Name of the file containing data to be read from the filesystem. */
#define SHARED_FILE_NAME "shared_data"

/* Name of the shared memory region. */
#define SHARED_FILE_BUFFER_NAME "data_buffer"

/* Name of the shared memory region for pending reads.
 * This shared memory region will store an array of integers that describe how many reads are
 * pending for a particular buffer slot. */
#define PENDING_READS_NAME "pending_reads"

/* Simulator output filename. */
#define SHARED_FILE_SIM_OUT_NAME "sim_out"

/* Name of the shared memory region containing shared configuration for readers and writers.
 * This includes semaphores, etc. as described by the RWConfig struct. */
#define SHARED_CONFIG_NAME "shared_config"

/* Size of the shared memory region containing shared configuration for readers and writers. */
#define SHARED_CONFIG_SIZE (sizeof(RWConfig))

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
    /* The buffer index writers are currently writing to. */
    int idxWrite;

    /* The program's command-line configuration. */
    ProgramConfig pConfig;

    /* The number of active readers. This is used by readers to ensure writers cannot write while
     * readers are reading. */
    int activeReaders;

    /* The number of writes performed. This is used by writers to determine when to terminate. */
    int writes;

    /* The number of readers waiting for a new buffer entry to read. */
    int emptyWaiters;

    /* The number of writers waiting for an empty buffer entry to write to. */
    int fullWaiters;

    /* Semaphore used to ensure mutual exclusion for emptyWaiters. */
    sem_t emptyWaitersSem;

    /* Semaphore used to ensure mutual exclusion for fullWaiters. */
    sem_t fullWaitersSem;

    /* Semaphore used to ensure mutual exclusion for activeReaders. */
    sem_t rcSem;

    /* Semaphore used to suspend readers until a buffer entry is written. */
    sem_t emptyCond;

    /* Semaphore used to suspend writers until a buffer entry is free. */
    sem_t fullCond;

    /* Semaphore used to ensure mutual exclusion of writers. */
    sem_t writeSem;

    /* Semaphore used to ensure mutual exclusion of pendingReads. */
    sem_t rpSem;
    
    /* Semaphore used to block readers if a writer is active, or block writers if a reader is active. */
    sem_t rwSem;
} RWConfig;

/* Creates the RWConfig, encapsulating the command line configuration. */
RWConfig createRWConfig(ProgramConfig);

/* Initializes an array with a default value. */
void initializeDefaultValueArray(int *array, int length, int value);

/* Opens a shared memory segment. */
void *openSharedMemory(char *name, int size);
int closeSharedMemory(char *name);

#endif /* ifndef SHARED_H */
