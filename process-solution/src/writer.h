#ifndef WRITER_H
#define WRITER_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "shared.h"
#include "simwrite.h"

/*
 * Performs a writer's responsibilities:
 * - Reads a single integer from a file.
 * - Writes the integer to a shared memory buffer, for readers to consume.
 * - Repeats as long as it has data to read.
 */
void writer();

#endif /* ifndef WRITER_H */
