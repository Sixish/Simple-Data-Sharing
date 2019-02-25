#include "shared.h"

/*
 * Opens shared memory.
 */
void *openSharedMemory(char *name, int size)
{
    int fd = shm_open(name, O_RDWR, 0666);
    void *ptr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    return ptr;
}

/*
 * Closes shared memory.
 *
 * This function is only provided to provide an alias for shm_unlink. Not required, but since we provide openSharedMemory
 * this function exists to free any resources that were created in openSharedMemory().
 */
int closeSharedMemory(char *name)
{
    return shm_unlink(name);
}

/*
 * Initializes an array of the specified size with default value dVal.
 */
void initializeDefaultValueArray(int *array, int size, int dVal)
{
    int i;

    for (i = 0; i < size; i++)
    {
        array[i] = dVal;
    }
}

/*
 * Creates the RW config given the program's configuration.
 * Initializes mutex locks and conditional variables.
 * Opens shared files.
 */
RWConfig createRWConfig(ProgramConfig pConfig)
{
    RWConfig config;

    /* Writers need to know the next buffer position to write to. */
    config.idxWrite = 0;

    /* Readers & Writers need to know how long to sleep for. Encapsulate the information within the RWConfig. */
    config.pConfig = pConfig;

    /* Initialize the number of readers reading from the buffer. Note that we don't need a corresponding
     * value for writers, because there can only be one. */
    config.activeReaders = 0;

    /* Initialize the number of writes. This will be used to determine when all values have been written. */
    config.writes = 0;

    config.emptyWaiters = 0;
    config.fullWaiters = 0;

    sem_init(&config.emptyCond, 1, 0);
    sem_init(&config.fullCond, 1, 0);

    sem_init(&config.emptyWaitersSem, 1, 1);
    sem_init(&config.fullWaitersSem, 1, 1);

    sem_init(&config.writeSem, 1, 1);
    sem_init(&config.rpSem, 1, 1);
    sem_init(&config.rcSem, 1, 1);
    sem_init(&config.rwSem, 1, 1);

    return config;
}

