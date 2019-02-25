#include "main.h"

/* 
 * Store all reader and writer processes in the following arrays.
 */
static pid_t *readers = NULL;
static pid_t *writers = NULL;

static void clearMemory()
{
    if (readers != NULL)
    {
        free(readers);
    }
    if (writers != NULL)
    {
        free(writers);
    }
}

/*
 * Creates a segment of shared memory with read-write access.
 *
 * Returns a memory map of the shared memory.
 *
 * To close the shared memory segment, call closeSharedMemory(char *).
 */
void *createSharedMemory(char *name, int size)
{
    int fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    ftruncate(fd, size);

    return mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
}

/*
 * Creates a set of num processes. Each process calls the specified callback with the provided argument.
 * Each created process' identifier is inserted sequentially into the passed array.
 */
int createProcesses(pid_t *array, int num, void (*callback) (void))
{
    int created = 0;
    while (created < num)
    {
        array[created] = fork();
        if (!array[created])
        {
            /*
             * Clear the redundant memory that the child process does not need. Since this was all
             * duplicated in the fork, it is useless to us now. Anything that is still relevant is stored
             * in shared memory, and is hence not duplicated. The parent process can handle that.
             */
            clearMemory();

            /* Child process: call the callback. */
            callback();
        }
        created++;
    }

    return created;
}

/*
 * Starts a set of writer procesess, inserting each writer process into the passed array.
 */
int startWriters(pid_t *array, RWConfig *config)
{
    return createProcesses(array, config->pConfig.writerCount, &writer);
}

/*
 * Starts a set of reader processes, inserting each reader process into the passed array.
 */
int startReaders(pid_t *array, RWConfig *config)
{
    return createProcesses(array, config->pConfig.readerCount, &reader);
}

/*
 * Reads an integer from a string.
 */
int readInt(char *str)
{
    int value = 0;
    sscanf(str, "%d", &value);

    return value;
}

/*
 * Prints a message to the console based on the status code.
 */
void printStatus(int sCode)
{
    char *message = NULL;
    switch (sCode)
    {
        default:
            message = "Completed successfully.";
            break;
    }

    printf("%s\n", message);
}

/*
 * Reads the command line arguments and forms a ProgramConfig structure based on them.
 *
 * The returned structure will contain values that may be needed for the functioning of the program,
 * but in an encapsulating struct.
 */
ProgramConfig readCommandLineArguments(char **argv)
{
    ProgramConfig config;

    /* Skip program name. */
    int idx = 1;

    /* Read the number of readers. */
    config.readerCount = readInt(argv[idx++]);
    /* Read the number of writers. */
    config.writerCount = readInt(argv[idx++]);
    /* Read the number of seconds to sleep for readers. */
    config.readerSleepTime = readInt(argv[idx++]);
    /* Read the number of seconds to sleep for writers. */
    config.writerSleepTime = readInt(argv[idx++]);

    return config;
}

/*
 * Entry point for the program.
 */
int main(int argc, char **argv)
{
    int sCode = 0, status, processes = 0, *data_buffer = NULL, *pendingReads = NULL;

    /*
     * Store the rwConfig as a global so children can free it.
     */
    RWConfig *rwConfig;

    /*
     * The program's command-line configuration.
     */
    ProgramConfig config;

    /* Create shared memory for the data_buffer.
     * 
     * This is based on the book's implementation of shm_open. The book has a typo, instead of O_RDWR it
     * says O_RDWR, which is meaningless.
     *
     * This is an array of values that we read from the file. Writers will add values to this, while
     * readers read values from it.
     */
    data_buffer = (int *)createSharedMemory(SHARED_FILE_BUFFER_NAME, SHM_BUFFER_SIZE * sizeof(int));

    /*
     * Create shared memory for the RWConfig.
     *
     * This will be used by all reader and writer processes. It contains a single RWConfig, which will
     * be used to communicate process configurations through the use of semaphores.
     */
    rwConfig = (RWConfig *)createSharedMemory(SHARED_CONFIG_NAME, SHARED_CONFIG_SIZE);

    /* 
     * Create shared memory for a list of pending reads.
     *
     * Reader processes will access this to decrement a pending read. Writer processes will access this
     * to determine if a buffer slot can be overwritten.
     */
    pendingReads = (int *)createSharedMemory(PENDING_READS_NAME, SHM_BUFFER_SIZE);

    if (argc >= MIN_NUM_CLARGS)
    {
        /* Overwrite the file that we are writing to. */
        simWriteClear();
        initializeDefaultValueArray(data_buffer, SHM_BUFFER_SIZE, -1);
        initializeDefaultValueArray(pendingReads, SHM_BUFFER_SIZE, 0);
        config = readCommandLineArguments(argv);
        *rwConfig = createRWConfig(config);

        readers = (pid_t *)malloc(config.readerCount * sizeof(pid_t));
        writers = (pid_t *)malloc(config.writerCount * sizeof(pid_t));

        /* Start the threads. */
        startReaders(readers, rwConfig);
        startWriters(writers, rwConfig);

        /* Wait for all threads to join the main thread of execution. */
        processes = config.readerCount + config.writerCount;
        while (processes--)
        {
            printf("Waiting for termination of process #%d / %d total.\n", processes,
                config.readerCount + config.writerCount);
            wait(&status);
            printf("Process terminated with code=%d\n", status);
            sCode = status || sCode;
        }
        clearMemory();
    }
    else
    {
        sCode = ERROR_TOO_FEW_ARGS || sCode;
    }

    /*
     * Close the shared memory segments.
     */
    closeSharedMemory(SHARED_FILE_BUFFER_NAME);
    closeSharedMemory(SHARED_CONFIG_NAME);
    closeSharedMemory(PENDING_READS_NAME);

    printStatus(sCode);

    return sCode;
}
