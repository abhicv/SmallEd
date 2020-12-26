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

File ReadFileFromDisk(const u8 *fileName)
{
    File file = {0};
    
    HANDLE fileHnd = NULL;
    fileHnd = CreateFileA(fileName,
                          GENERIC_READ,
                          FILE_SHARE_READ,
                          NULL,
                          OPEN_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL);
    
    if(fileHnd != INVALID_HANDLE_VALUE)
    {
        u32 size = 0;
        size = GetFileSize(fileHnd, NULL);
        
        u32 readBytes = 0;
        
        file.buffer = 0;
        file.buffer = (u8*)malloc(size);
        
        if(file.buffer)
        {
            ReadFile(fileHnd,
                     file.buffer,
                     size,
                     &readBytes,
                     NULL);
        }
        
        if(readBytes == size)
        {
            printf("Read all bytes from '%s'\n", fileName);
        }
        else
        {
            printf("error reading bytes from '%s'\n", fileName);
            
            free(file.buffer);
            file.loaded = false;
            
            return file;
        }
        
        CloseHandle(fileHnd);
        file.size = size;
        file.loaded = true;
        
        return file;
    }
    else
    {
        printf("Failed to open the file '%s'\n", fileName);
        file.loaded = false;
        return file;
    }
}

u32 GetLineCount(u8 *fileBuffer, u32 size)
{
    u32 lineCount = 0;
    
    for(u32 n = 0; n < size; n++)
    {
        if(fileBuffer[n] == '\n')
        {
            lineCount++;
        }
    }
    lineCount++;
    
    return lineCount;
}

#endif //SMALLED_FILE_H
