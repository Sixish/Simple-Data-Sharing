#include "simwrite.h"

int simWriteFinish(char *type, char *action, char *dest, int id, int reads)
{
    int sCode = 0;
    FILE *fPtr = fopen(SHARED_FILE_SIM_OUT_NAME, "a");
    if (fPtr)
    {
        if (fprintf(fPtr, "%s-%d has finished %s %d pieces of data %s the data_buffer.\n",
            type, id, action, reads, dest) < 0)
        {
            /*
             * fprintf returned a negative value, this indicates something went wrong when writing to file.
             * Not much we can do in this case. Save this for returning.
             */
            sCode = ERROR_WRITING_FILE;
        }

        if (fclose(fPtr))
        {
            /*
             * The file could not be closed. Odd.
             */
            sCode = ERROR_CLOSING_FILE;
        }
    }
    else
    {
        /*
         * Not much we can do if the file opening process failed.
         */
        sCode = ERROR_OPENING_FILE;
    }

    return sCode;
}

/*
 * Clears a file, removing its contents.
 */
int simWriteClear()
{
    int sCode = 0;
    FILE *fPtr = fopen(SHARED_FILE_SIM_OUT_NAME, "w");
    if (fPtr)
    {
        if (fclose(fPtr))
        {
            /*
             * The file could not be closed.
             */
            sCode = ERROR_CLOSING_FILE;
        }
    }
    else
    {
        /*
         * The file could not be opened.
         */
        sCode = ERROR_OPENING_FILE;
    }

    return sCode;
}
