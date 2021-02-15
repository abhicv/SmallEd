#ifndef SMALLED_FILE_H
#define SMALLED_FILE_H

#include "types.h"
#include "Windows.h"
#include "smalled_memory.h"

typedef struct File
{
    u8 *buffer;
    u32 size;
    b32 loaded;
    
    u8 *name;
    
} File;

typedef struct FileList
{
    //can list upto 100 files
    WIN32_FIND_DATAA fileDatas[100];
    u32 fileCount;
    u32 selectedIndex;
    u8 currentDir[250];
    
} FileList;

File ReadFileFromDisk(const u8 *fileName);
u32 GetLineCount(u8 *fileBuffer, u32 size);

#endif //SMALLED_FILE_H
