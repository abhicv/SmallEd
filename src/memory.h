#ifndef MEMORY_H
#define MEMORY_H

#include "types.h"

#define KiloByte(value) value * 1024
#define MegaByte(value) KiloByte(value) * 1024
#define GigaByte(value) MegaByte(value) * 1024

typedef struct Memory
{
    void *permanentStorage;
    u64 permanentStorageSize;
    
} Memory;

void* AllocateMemory(Memory *memory, u64 size)
{
    void *ptr = memory->permanentStorage;
    memory->permanentStorage = (u8*)memory->permanentStorage + size;
    return ptr;
}

#endif //MEMORY_H
