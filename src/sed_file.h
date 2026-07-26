#ifndef SED_FILE_H
#define SED_FILE_H

#include <Windows.h>

#include "sed_font.h"
#include "sed_memory.h"
#include "sed_types.h"

#define MAX_FILES_LISTABLE 100
#define MAX_FILE_PATH_SIZE 512

typedef struct FileList
{
    WIN32_FIND_DATAA files[MAX_FILES_LISTABLE];
    u32 fileCount;
    u32 selectedIndex;
    u8 currentDir[MAX_FILE_PATH_SIZE];
    FontData *fileIconFont;
} FileList;

u8 *ReadFileIntoMemory(const u8 *fileName);
void WriteFileU8(const u8 *fileName, u8 *buffer, u32 size);
void GoUpDirectory(FileList *fileList);
void ListFileInDirectory(FileList *fileList);

#endif // SED_FILE_H
