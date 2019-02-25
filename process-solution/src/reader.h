#ifndef READER_H
#define READER_H

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include "shared.h"

#include "simwrite.h"

void reader();

#endif /* ifndef READER_H */
