#ifndef SIMWRITE_H
#define SIMWRITE_H

#include <stdio.h>
#include <unistd.h>
#include "shared.h"

#define ERROR_WRITING_FILE -733
#define ERROR_CLOSING_FILE -734
#define ERROR_OPENING_FILE -735

int simWriteFinish(char *type, char *action, char *dest, int id, int val);
int simWriteClear();

#endif /* ifndef SIMWRITE_H */
