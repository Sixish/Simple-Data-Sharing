#include "shared.h"

/*
 * Creates an initializes an array of the specified size with default value dVal. Returns the created array.
 */
int *createDefaultValueArray(int size, int dVal)
{
    int i, *data = (int *)malloc(size * sizeof(int));

    for (i = 0; i < size; i++)
    {
        data[i] = dVal;
    }

    return data;
}

/*
 * Creates the RW config given the program's configuration.
 * Initializes mutex locks and conditional variables.
 * Opens shared files.
 */
RWConfig *createRWConfig(ProgramConfig *pConfig)
{
    RWConfig *config = (RWConfig *)malloc(sizeof(RWConfig));

    /* Readers & Writers need to know how long to sleep for. Encapsulate the information within the RWConfig. */
    config->pConfig = pConfig;

    /* Create the shared memory buffer. */
    config->data = createDefaultValueArray(SHM_BUFFER_SIZE, -1);

    /* Initialize the number of readers reading from the buffer. Note that we don't need a corresponding
     * value for writers, because there can only be one. */
    config->activeReaders = 0;

    /* Initialize the number of writes. This will be used to determine when all values have been written. */
    config->writes = 0;

    /* Initialize the mutexes we require to ensure synchronisation. */
    pthread_mutex_init(&config->writeMutex, NULL);
    pthread_cond_init(&config->emptyCond, NULL);
    pthread_cond_init(&config->fullCond, NULL);
    pthread_mutex_init(&config->rcMutex, NULL);
    pthread_mutex_init(&config->rpMutex, NULL);
    pthread_mutex_init(&config->rwMutex, NULL);

    /* For threads, share a common file resource - much more effective than creating opening the file per
     * thread. */
    config->fPtrSimOut = fopen("sim_out", "w");

    /* Create a file reader for the shared data. */
    config->fPtrSharedData = fopen(SHARED_FILE_NAME, "r");

    /* We want to ensure the read counts are initialised to 0. This prevents readers from reading before
     * any writers have written data to the buffers. */
    config->pendingReads = (int *)calloc(SHM_BUFFER_SIZE, sizeof(int));

    /* Start writers from the start of the buffer. */
    config->idxWrite = 0;

    return config;
}

/*
 * Frees all resources associated with a RWConfig instance.
 */
void freeRWConfig(RWConfig *config)
{
    free(config->data);
    free(config->pConfig);
    free(config->pendingReads);
    fclose(config->fPtrSimOut);
    fclose(config->fPtrSharedData);
    free(config);
}

/*
 * Creates a dynamic integer on the heap for a persistent return value.
 */
int *ret(int val)
{
    int *mem = (int *)malloc(sizeof(int));
    *mem = val;

    return mem;
}

/*
 * Writes to file that a thread has finished its task.
 */
void simWriteFinish(FILE *fPtr, char *type, char *action, char *dest, int id, int reads)
{
    fprintf(fPtr, "%s-%u has finished %s %d pieces of data %s the data_buffer.\n",
        type, id, action, reads, dest);
}

