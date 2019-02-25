#include "writer.h"

/*
 * Reads a single data item from the file.
 */
static int readNextDataItem(RWConfig *rwConfig)
{
    int val = -1;

    fscanf(rwConfig->fPtrSharedData, "%d", &val);

    return val;
}

/*
 * Writes to a shared memory buffer the values read from a file.
 */
void *writer(void *vpConfig)
{
    RWConfig *rwConfig = (RWConfig *)vpConfig;
    int value, *data = rwConfig->data, selfWrites = 0;
    bool done = false;

    while (!done)
    {
        /*
         * This loop is required in order to repeatedly acquire a mutex lock. Without it, synchronisation
         * issues may occur and writers may overwrite to the buffer.
         *
         * We cannot simply write
         *     while (rwConfig->writes < NUM_ELEMENTS_DATA_FILE) ...
         * because:
         * 1. We need to acquire a mutex lock in order to prevent synchronisation issues of rwConfig->writes.
         * 2. Each writer must be able to cooperate, i.e. a writer may release the mutex lock after each
         *    write, not only after it is completely exhausted the total write count.
         */

        /* Only allow one writer to read/write to the writer count simultaneously. */
        pthread_mutex_lock(&rwConfig->writeMutex);

        /*
         * It's possible that the writer encounters a buffer that has not been fully read. In this case,
         * we need to wait until a number of readers read from the buffer. We will respond to each
         * signal, until we eventually find rwConfig->pendingReads[idx] to be 0.
         *
         * We are guaranteed to eventually reach this state as long as there exists a reader, because
         * readers will only wait if they cannot read the buffer slot they are up to, but this condition
         * only occurs if all buffer slots have been fully read; the conditions for waiting are mutually
         * exclusive, so no deadlock can occur.
         */
        pthread_mutex_lock(&rwConfig->rpMutex);
        while (rwConfig->pendingReads[rwConfig->idxWrite])
        {
            /*
             * We cannot write because a reader is waiting to read this. Wait until a reader reports that
             * it has finished reading, and check again.
             *
             * On each wait, we need to release the mutex for the pending reads, because otherwise the
             * readers will be stuck in a deadlock trying to acquire this mutex lock.
             */
            pthread_cond_wait(&rwConfig->fullCond, &rwConfig->rpMutex);
        }
        pthread_mutex_unlock(&rwConfig->rpMutex);

        pthread_mutex_lock(&rwConfig->rwMutex);

        if (rwConfig->writes < NUM_ELEMENTS_DATA_FILE)
        {

            /* Read value from file. */
            value = readNextDataItem(rwConfig);

            /*
             * Place value in buffer. Since only one writer can be executing in its critical section
             * simultaneously, rwConfig->idxWrite is guaranteed to be synchronised, as only writers
             * access this value. Variables rwConfig->writes, selfWrites must also be updated; these are
             * also synchronised for the same reason. 
             */
            data[rwConfig->idxWrite] = value;
            rwConfig->writes++;
            selfWrites++;
            printf("Write #%d/%d (%d) to data buffer index %d\n", selfWrites, rwConfig->writes, value,
                rwConfig->idxWrite);

            /*
             * Reset the pending reads to ensure readers can begin reading again. We still have the mutex
             * lock rwConfig->rpMutex, so we can do this.
             *
             * On resetting, release the mutex lock. We don't need to change pendingReads again.
             */
            pthread_mutex_lock(&rwConfig->rpMutex);
            rwConfig->pendingReads[rwConfig->idxWrite] = rwConfig->pConfig->readerCount;
            pthread_mutex_unlock(&rwConfig->rpMutex);

            /*
             * Writers must progress to the next buffer slot. This is bound to rwConfig->writeMutex, which
             * ensures that only one writer will ever access rwConfig->idxWrite is accessed simultaneously.
             */
            rwConfig->idxWrite = (rwConfig->idxWrite + 1) % SHM_BUFFER_SIZE;

        }
        pthread_mutex_unlock(&rwConfig->rwMutex);

        /* If we've reached the end, signal that we are done. */
        done = !(rwConfig->writes < NUM_ELEMENTS_DATA_FILE);
        pthread_mutex_unlock(&rwConfig->writeMutex);

        /*
         * If any readers were waiting because their buffers were empty (fully read), then we need to
         * wake them up.
         */
        pthread_cond_signal(&rwConfig->emptyCond);

        /* Per specification: sleep after writing and updating the counter. */
        sleep(rwConfig->pConfig->writerSleepTime);
    }

    /*
     * Write the number of writes to file.
     *
     * Per discussion with Soh: For multithreading solution, use thread ID instead of process ID.
     */
    simWriteFinish(rwConfig->fPtrSimOut, "writer", "writing", "to", pthread_self(), selfWrites);

    /*
     * While we could call pthread_exit(0), it is equivalent to return. Pthreads will still clean up
     * the resources.
     */
    return ret(selfWrites);
}
