#ifndef SED_MEMORY_H
#define SED_MEMORY_H

#include "assert.h"
#include "sed_types.h"

#define KiloByte(value) value * 1024LL
#define MegaByte(value) KiloByte(value) * 1024LL
#define GigaByte(value) MegaByte(value) * 1024LL

typedef struct MemoryArena
{
    void *persistentStorageBase;
    u64 persistentStorageOffset;
    u64 persistentStorageSize;

    void *transientStorageBase;
    u64 transientStorageOffset;
    u64 transientStorageSize;

} MemoryArena;

global MemoryArena arena;

void InitGlobalMemoryArena();
void *AllocatePersistentMemory(u64 size);
void *AllocateTransientMemory(u64 size);
void ResetPersistentMemory();
void ResetTransientMemory();

#endif // SED_MEMORY_H
