#ifndef SED_FILE_H
#define SED_FILE_H

#include <Windows.h>

#include "sed_memory.h"
#include "sed_types.h"

typedef struct File
{
    u8 *buffer;
    u32 size;
    b32 loaded;
    u8 *name;
    
} File;

#define MAX_FILES_LISTABLE 100
#define MAX_FILE_PATH_SIZE 512

typedef struct FileList
{
    WIN32_FIND_DATAA files[MAX_FILES_LISTABLE];
    u32 fileCount;
    u32 selectedIndex;
    u8 currentDir[MAX_FILE_PATH_SIZE];
    
} FileList;

File ReadFileNullTerminate(const u8 *fileName);

#endif //SED_FILE_H
