#ifndef SED_MEMORY_H
#define SED_MEMORY_H

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

void InitGlobalMemoryArena()
{
    arena.persistentStorageOffset = 0;
    arena.persistentStorageSize = MegaByte(500);
    arena.transientStorageOffset = 0;
    arena.transientStorageSize = MegaByte(64);
    
    arena.persistentStorageBase = VirtualAlloc(0, arena.persistentStorageSize + arena.transientStorageSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    
    arena.transientStorageBase = (u8*)arena.persistentStorageBase + arena.persistentStorageSize;
}

void* AllocatePersistentMemory(u64 size)
{
    if(arena.persistentStorageOffset + size < arena.persistentStorageSize)
    {
        void *ptr = (u8*)arena.persistentStorageBase + arena.persistentStorageOffset;
        arena.persistentStorageOffset += size;
        return ptr;
    }
    else
    {
        printf("Persistent memory full!\n");
        return 0;
    }
}

void* AllocateTransientMemory(u64 size)
{
    if(arena.transientStorageOffset + size < arena.transientStorageSize)
    {
        void *ptr = (u8*)arena.transientStorageBase + arena.transientStorageOffset;
        arena.transientStorageOffset += size;
        return ptr;
    }
    else
    {
        printf("Transient memory full!\n");
        return 0;
    }
}

void ResetPersistentMemory()
{
    arena.persistentStorageSize = 0;
}

void ResetTransientMemory()
{
    arena.transientStorageSize = 0;
}

#endif //SED_MEMORY_H

