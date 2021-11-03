#include "debug.h"

#include <stdio.h>

void disassembleChunk(Chunk *chunk, const char *name) {
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->size();) {
        offset = chunk->disassembleInstruction(offset);
    }
}
