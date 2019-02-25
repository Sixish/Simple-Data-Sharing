#include "reader.h"

/*
 * Reader thread callback.
 *
 * Reads all items from a buffer, or waits if the buffer doesn't have anything to read.
 */
void *reader(void *vpConfig)
{
    RWConfig *rwConfig = (RWConfig *)vpConfig;
    int reads = 0, *cache, *data = rwConfig->data, idx = 0, value;

    /* The cache stores the value that we saw at this position last time it was read. If we encounter a
     * value that hasn't changed, then we should not read it but instead wait for writers to write.
     * This is used to work around the possibility that a single reader attempts to read more than one
     * set of values from the buffer before the writer can replace them. */
    cache = createDefaultValueArray(SHM_BUFFER_SIZE, -2);

    /* Current read position in the circular queue. */
    while (reads < NUM_ELEMENTS_DATA_FILE)
    {
        /* Ensure that we aren't reading the same data. If we are, then the writers have not yet
         * written to this slot. Wait until a writer does before continuing.
         *
         * We could also be checking if the number of pending readers isn't 0, but it's guaranteed not to be 0
         */
        pthread_mutex_lock(&rwConfig->rpMutex);
        while (cache[idx] == data[idx] || !rwConfig->pendingReads[idx])
        {
            pthread_cond_wait(&rwConfig->emptyCond, &rwConfig->rpMutex);
        }
        pthread_mutex_unlock(&rwConfig->rpMutex);

        /*
         * Increment the number of readers currently reading. Since multiple readers may perform this
         * simultaneously, it must be constrained by a mutex lock. If this reader happens to be the first
         * reader to increment the count, then wait for the writer to finish by requesting the rwMutex.
         * This also ensures that writers do not attempt to write while the buffer is being read from.
         */
        pthread_mutex_lock(&rwConfig->rcMutex);
        if (!rwConfig->activeReaders)
        {
            /* Wait for any writers to finish. */
            pthread_mutex_lock(&rwConfig->rwMutex);
        }
        rwConfig->activeReaders++;
        pthread_mutex_unlock(&rwConfig->rcMutex);

        value = data[idx];
        cache[idx] = value;
        printf("Read value #%d (%d) from data buffer index %d.\n", reads, value, idx);
        reads++;

        /* Request a mutex lock to avoid synchronisation issues of pending read counts. */
        pthread_mutex_lock(&rwConfig->rpMutex);
        rwConfig->pendingReads[idx]--;
        pthread_mutex_unlock(&rwConfig->rpMutex);

        idx = (idx + 1) % SHM_BUFFER_SIZE;

        /*
         * Decrement the number of readers currently reading. Since multiple readers may perform this
         * simultaneously, we must first request a mutex lock. If this reader is the final reader to read
         * from the buffer, then unlock the write mutex and hence enable writers to write again.
         */
        pthread_mutex_lock(&rwConfig->rcMutex);
        rwConfig->activeReaders--;
        if (!rwConfig->activeReaders)
        {
            pthread_mutex_unlock(&rwConfig->rwMutex);
        }
        pthread_mutex_unlock(&rwConfig->rcMutex);

        /*
         * Wake up any writers that went to sleep because there were no empty buffers to write to.
         */
        pthread_cond_signal(&rwConfig->fullCond);

        /*
         * All this reading has made me tired. Time for a well-earned nap.
         */
        sleep(rwConfig->pConfig->readerSleepTime);
    }

    /*
     * Save the write count to file.
     *
     * Per discussion with Soh: use thread ID (pthread_self()) instead of process ID for multithreading
     * solution.
     */
    simWriteFinish(rwConfig->fPtrSimOut, "reader", "reading", "from", pthread_self(), reads);

    free(cache);
    return ret(reads);
}
