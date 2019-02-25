#include "writer.h"

/*
 * Reads a single data item from the file.
 */
static int readNextDataItem(RWConfig *rwConfig)
{
    int rRemaining = rwConfig->writes + 1, val;
    FILE *fPtrSharedData = fopen(SHARED_FILE_NAME, "r");

    /*
     * Since processes do not share resources such as files, we need to consume the integers that other
     * writers have already read from the file.
     */
    while (rRemaining--)
    {
        fscanf(fPtrSharedData, "%d", &val);
    }

    fclose(fPtrSharedData);
    return val;
}

/*
 * Writes to a shared memory buffer the values read from a file.
 */
void writer()
{
    RWConfig *rwConfig = NULL;
    int value, *data = NULL, selfWrites = 0, *pendingReads;
    bool done = false;

    data = (int *)openSharedMemory(SHARED_FILE_BUFFER_NAME, SHM_BUFFER_SIZE);

    rwConfig = (RWConfig *)openSharedMemory(SHARED_CONFIG_NAME, SHARED_CONFIG_SIZE);

    pendingReads = openSharedMemory(PENDING_READS_NAME, SHM_BUFFER_SIZE);

    while (!done)
    {
        /* This loop is required in order to repeatedly acquire a mutex lock. Without it, synchronisation
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
        sem_wait(&rwConfig->writeSem);

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
        sem_wait(&rwConfig->rpSem);
        while (pendingReads[rwConfig->idxWrite])
        {
            /*
             * We cannot write because a reader is waiting to read this. Wait until a reader reports that
             * it has finished reading, and check again.
             *
             * On each wait, we need to release the mutex for the pending reads, because otherwise the
             * readers will be stuck in a deadlock trying to acquire this mutex lock.
             */
            sem_post(&rwConfig->rpSem);

            /*
             * Ensure that no other processes can change fullWaiters while we are attempting to increment it.
             *
             * We need this to ensure readers sem_post fullCond often enough to allow all writers a chance to
             * check. Otherwise, we may eventually run out of readers to read, causing a deadlock.
             *
             * After incrementing, put the process to sleep until awoken by a writer's sem_post.
             */
            sem_wait(&rwConfig->fullWaitersSem);
            rwConfig->fullWaiters++;
            sem_post(&rwConfig->fullWaitersSem);
            sem_wait(&rwConfig->fullCond);

            sem_wait(&rwConfig->rpSem);
        }
        sem_post(&rwConfig->rpSem);

        sem_wait(&rwConfig->rwSem);

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
            sem_wait(&rwConfig->rpSem);
            pendingReads[rwConfig->idxWrite] = rwConfig->pConfig.readerCount;
            sem_post(&rwConfig->rpSem);

            /*
             * Writers must progress to the next buffer slot. This is bound to rwConfig->writeMutex, which
             * ensures that only one writer will ever access rwConfig->idxWrite is accessed simultaneously.
             */
            rwConfig->idxWrite = (rwConfig->idxWrite + 1) % SHM_BUFFER_SIZE;

        }
        sem_post(&rwConfig->rwSem);

        /* If we've reached the end, signal that we are done. */
        done = !(rwConfig->writes < NUM_ELEMENTS_DATA_FILE);
        sem_post(&rwConfig->writeSem);

        /*
         * If any readers were waiting because their buffers were empty (fully read), then we need to
         * wake them up.
         *
         * This relies on the emptyWaiters being synchronised correctly. As such, we need a semaphore
         * to provide mutual exclusion.
         */
        sem_wait(&rwConfig->emptyWaitersSem);
        while (rwConfig->emptyWaiters > 0)
        {
            sem_post(&rwConfig->emptyCond);
            rwConfig->emptyWaiters--;
        }
        sem_post(&rwConfig->emptyWaitersSem);

        /* Per specification: sleep after writing and updating the counter. */
        sleep(rwConfig->pConfig.writerSleepTime);
    }

    /*
     * Write the number of writes to file.
     *
     * Per discussion with Soh: For multithreading solution, use thread ID instead of process ID.
     */
    simWriteFinish("writer", "writing", "to", getpid(), selfWrites);

    exit(0);
}
