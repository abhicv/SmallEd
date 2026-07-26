#include "sed_memory.h"

void InitGlobalMemoryArena()
{
    arena.persistentStorageOffset = 0;
    arena.persistentStorageSize = MegaByte(500);
    arena.transientStorageOffset = 0;
    arena.transientStorageSize = MegaByte(64);

    arena.persistentStorageBase = VirtualAlloc(0, arena.persistentStorageSize + arena.transientStorageSize,
                                               MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    arena.transientStorageBase = (u8 *)arena.persistentStorageBase + arena.persistentStorageSize;
}

void *AllocatePersistentMemory(u64 size)
{
    assert((arena.persistentStorageOffset + size) < arena.persistentStorageSize);
    void *ptr = (u8 *)arena.persistentStorageBase + arena.persistentStorageOffset;
    arena.persistentStorageOffset += size;
    return ptr;
}

void *AllocateTransientMemory(u64 size)
{
    assert(arena.transientStorageOffset + size < arena.transientStorageSize);
    void *ptr = (u8 *)arena.transientStorageBase + arena.transientStorageOffset;
    arena.transientStorageOffset += size;
    return ptr;
}

void ResetPersistentMemory()
{
    arena.persistentStorageSize = 0;
}

void ResetTransientMemory()
{
    arena.transientStorageSize = 0;
}
