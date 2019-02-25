#include "reader.h"

/*
 * Reader process callback.
 *
 * Reads all items from a buffer, or waits if the buffer doesn't have anything to read.
 */
void reader()
{
    int sCode = 0, status, *data, reads = 0, *cache = NULL, idx = 0, value, *pendingReads;

    /* Open shared memory to the shared semaphore states. */
    RWConfig *rwConfig = (RWConfig *)openSharedMemory(SHARED_CONFIG_NAME, SHARED_CONFIG_SIZE);

    /* Open shared memory to the data_buffer. */
    data = (int *)openSharedMemory(SHARED_FILE_BUFFER_NAME, SHM_BUFFER_SIZE);

    /* Open shared memory to the pending reads - number of reads for each buffer slot. */
    pendingReads = (int *)openSharedMemory(PENDING_READS_NAME, SHM_BUFFER_SIZE);

    /* The cache stores the value that we saw at this position last time it was read. If we encounter a
     * value that hasn't changed, then we should not read it but instead wait for writers to write.
     * This is used to work around the possibility that a single reader attempts to read more than one
     * set of values from the buffer before the writer can replace them. */
    cache = (int *)malloc(SHM_BUFFER_SIZE * sizeof(int));

    /* Initialize the cache entries to -2. */
    initializeDefaultValueArray(cache, SHM_BUFFER_SIZE, -2);

    while (reads < NUM_ELEMENTS_DATA_FILE)
    {
        /* Ensure that we aren't reading the same data. If we are, then the writers have not yet
         * written to this slot. Wait until a writer does before continuing.
         *
         * We could also be checking if the number of pending readers isn't 0, but it's guaranteed not to be 0
         */
        sem_wait(&rwConfig->rpSem);
        while (cache[idx] == data[idx] || !pendingReads[idx])
        {
            /*
             * Repeatedly unlock the pending reads semaphore until we are certain that we can continue.
             *
             * That is, wait until there exists new data in data[idx].
             */
            sem_post(&rwConfig->rpSem);

            /*
             * Ensure that no other processes can change emptyWaiters while we are attempting to increment it.
             *
             * We need this to ensure writers sem_post emptyCond often enough to allow all readers a chance to
             * check. Otherwise, we may eventually run out of writers to write.
             *
             * After incrementing, put the process to sleep until awoken by a reader's sem_post.
             */
            sem_wait(&rwConfig->emptyWaitersSem);
            rwConfig->emptyWaiters++;
            sem_post(&rwConfig->emptyWaitersSem);
            sem_wait(&rwConfig->emptyCond);

            sem_wait(&rwConfig->rpSem);
        }
        sem_post(&rwConfig->rpSem);

        /*
         * Increment the number of readers currently reading. Since multiple readers may perform this
         * simultaneously, it must be constrained by a mutex lock. If this reader happens to be the first
         * reader to increment the count, then wait for the writer to finish by requesting the rwMutex.
         * This also ensures that writers do not attempt to write while the buffer is being read from.
         */
        sem_wait(&rwConfig->rcSem);
        if (!rwConfig->activeReaders)
        {
            /* Wait for any writers to finish. */
            sem_wait(&rwConfig->rwSem);
        }
        rwConfig->activeReaders++;
        sem_post(&rwConfig->rcSem);

        value = data[idx];
        reads++;
        printf("Read value #%d/%d (%d) from data buffer index %d.\n", reads, NUM_ELEMENTS_DATA_FILE,
            value, idx);

        
        /*
         * Assign the value to cache so that we can detect full cycles.
         */
        cache[idx] = value;

        /* Request a mutex lock to avoid synchronisation issues of pending read counts. */
        sem_wait(&rwConfig->rpSem);
        pendingReads[idx]--;
        sem_post(&rwConfig->rpSem);

        idx = (idx + 1) % SHM_BUFFER_SIZE;

        /*
         * Decrement the number of readers currently reading. Since multiple readers may perform this
         * simultaneously, we must first request a mutex lock. If this reader is the final reader to read
         * from the buffer, then unlock the write mutex and hence enable writers to write again.
         */
        sem_wait(&rwConfig->rcSem);
        rwConfig->activeReaders--;
        if (!rwConfig->activeReaders)
        {
            sem_post(&rwConfig->rwSem);
        }
        sem_post(&rwConfig->rcSem);

        /*
         * Wake up any writers that went to sleep because there were no empty buffers to write to.
         *
         * This relies on the fullWaiters being synchronised correctly. As such, we need a semaphore
         * to provide mutual exclusion.
         */
        sem_wait(&rwConfig->fullWaitersSem);
        while (rwConfig->fullWaiters > 0)
        {
            sem_post(&rwConfig->fullCond);
            rwConfig->fullWaiters--;
        }
        sem_post(&rwConfig->fullWaitersSem);

        /*
         * All this reading has made me tired. Time for a well-earned nap.
         */
        sleep(rwConfig->pConfig.readerSleepTime);
    }
    free(cache);

    /*
     * Save the write count to file.
     */
    simWriteFinish("reader", "reading", "from", getpid(), reads);

    exit(sCode);
}
