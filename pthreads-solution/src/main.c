#include "main.h"

/*
 * Creates a set of num threads. Each thread calls the specified callback with the provided argument.
 * Each created thread is inserted sequentially into the passed array.
 */
int createThreads(pthread_t *array, int num, void *(*callback) (void *), void *arg)
{
    int created = 0;
    while (created < num && !pthread_create(&array[created], NULL, callback, arg))
    {
        created++;
    }

    return created;
}

/*
 * Starts a set of writer threads, inserting each writer thread into the passed array.
 */
int startWriters(pthread_t *array, RWConfig *config)
{
    return createThreads(array, config->pConfig->writerCount, &writer, config);
}

/*
 * Starts a set of reader threads, inserting each reader thread into the passed array.
 */
int startReaders(pthread_t *array, RWConfig *config)
{
    return createThreads(array, config->pConfig->readerCount, &reader, config);
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
 * Joins an array of threads of length count.
 *
 * Returns the error code of the last error to occur while joining, or 0 if no errors were encountered.
 */
void joinThreads(pthread_t *threads, int count, void **retValues)
{
    int idx;
    /* Wait for all threads to terminate. This allows us to clean up the resources they are consuming. */
    for (idx = 0; idx < count; idx++)
    {
        /* Wait for this thread to terminate so that we can join the threads.
         *
         * If joining the thread causes an error, save it to sCode for returning. If, on the other hand,
         * no errors occur, revert to the current sCode which may or may not be 0. */
        pthread_join(threads[idx], &retValues[idx]);
    }
}

/*
 * Joins an array of writer threads of length count.
 *
 * Returns a status code:
 *   ERROR_INCORRECT_WRITES:
 *     The threads failed to write the correct number of times in total.
 *   0:
 *     No errors were encountered.
 */
int joinWriterThreads(pthread_t *threads, int count)
{
    int i, sCode = 0, sum = 0, **retValues = (int **)malloc(count * sizeof(int *));

    joinThreads(threads, count, (void **)retValues);

    for (i = 0; i < count; i++)
    {
        sum += *retValues[i];
        free(retValues[i]);
    }

    if (sum != NUM_ELEMENTS_DATA_FILE)
    {
        sCode = ERROR_INCORRECT_WRITES;
        printf("Error: Incorrect number of total writes: %d\n", sum);
    }
    free(retValues);

    return sCode;
}

/*
 * Joins an array of reader threads of length count.
 *
 * Returns a status code:
 *   ERROR_INCORRECT_READS:
 *     At least one reader failed to read all elements added to the shared memory.
 *  0:
 *    No errors were encountered.
 */
int joinReaderThreads(pthread_t *threads, int count)
{
    int i, sCode = 0, **retValues = (int **)malloc(count * sizeof(int *));

    joinThreads(threads, count, (void **)retValues);

    for (i = 0; i < count; i++)
    {
        if (*retValues[i] != NUM_ELEMENTS_DATA_FILE)
        {
            sCode = ERROR_INCORRECT_READS || sCode;
            printf("Error: Incorrect number of reads: %d\n", *retValues[i]);
        }
        free(retValues[i]);
    }
    free(retValues);

    return sCode;
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
ProgramConfig *readCommandLineArguments(char **argv)
{
    ProgramConfig *config = (ProgramConfig *)malloc(sizeof(ProgramConfig));

    /* Skip program name. */
    int idx = 1;

    /* Read the number of readers. */
    config->readerCount = readInt(argv[idx++]);
    /* Read the number of writers. */
    config->writerCount = readInt(argv[idx++]);
    /* Read the number of ms to sleep for readers. */
    config->readerSleepTime = readInt(argv[idx++]);
    /* Read the number of ms to sleep for writers. */
    config->writerSleepTime = readInt(argv[idx++]);

    return config;
}

/*
 * Entry point for the program.
 */
int main(int argc, char **argv)
{
    int sCode = 0;
    ProgramConfig *config;

    RWConfig *rwConfig;

    /* Reader & Writer threads. */
    pthread_t *readers = NULL;
    pthread_t *writers = NULL;

    if (argc >= MIN_NUM_CLARGS)
    {
        config = readCommandLineArguments(argv);

        readers = (pthread_t *)malloc(config->readerCount * sizeof(pthread_t));
        writers = (pthread_t *)malloc(config->writerCount * sizeof(pthread_t));

        rwConfig = createRWConfig(config);

        /* Start the threads. */
        startReaders(readers, rwConfig);
        startWriters(writers, rwConfig);

        /* Wait for all threads to join the main thread of execution. */
        sCode = joinReaderThreads(readers, config->readerCount) || sCode;
        sCode = joinWriterThreads(writers, config->writerCount) || sCode;

        freeRWConfig(rwConfig);
    }
    else
    {
        sCode = ERROR_TOO_FEW_ARGS || sCode;
    }

    printStatus(sCode);

    free(readers);
    free(writers);
    return sCode;
}
