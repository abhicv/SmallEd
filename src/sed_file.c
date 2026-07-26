#include "sed_file.h"

u8 *ReadFileIntoMemory(const u8 *fileName)
{
    HANDLE fileHnd =
        CreateFileA(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (fileHnd == INVALID_HANDLE_VALUE)
    {
        printf("Failed to open the file '%s'\n", fileName);
        return 0;
    }

    u32 size = GetFileSize(fileHnd, NULL);

    u8 *buffer = (u8 *)AllocatePersistentMemory(sizeof(u8) * (size + 1));

    u32 readBytes = 0;
    ReadFile(fileHnd, buffer, size, &readBytes, NULL);

    if (readBytes != size)
    {
        printf("Failed to read bytes from file %s', total read = %d, total size = %d\n", fileName, readBytes, size);
        CloseHandle(fileHnd);
        return 0;
    }

    buffer[size] = 0;

    return buffer;
}

void WriteFileU8(const u8 *fileName, u8 *buffer, u32 size)
{
    HANDLE fileHnd =
        CreateFileA(fileName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (fileHnd != INVALID_HANDLE_VALUE)
    {
        u32 bytesWriten = 0;

        WriteFile(fileHnd, buffer, size, &bytesWriten, NULL);

        if (size == bytesWriten)
        {
            printf("File write success!\n");
        }
        else
        {
            printf("File write failed!\n");
        }

        CloseHandle(fileHnd);
    }
}

void GoUpDirectory(FileList *fileList)
{
    u32 n = strlen(fileList->currentDir);

    while (fileList->currentDir[n] != '\\')
    {
        n--;
        if (n == 3)
        {
            break;
        }
    }

    fileList->currentDir[n] = 0;
}

void ListFileInDirectory(FileList *fileList)
{
    fileList->fileCount = 0;
    fileList->selectedIndex = 2;

    u32 size = strlen(fileList->currentDir);
    // printf("dir: %s\n", fileList->currentDir);

    u8 *pathStr = (u8 *)malloc(size + 3);
    memcpy(pathStr, fileList->currentDir, size);
    pathStr[size] = '\\';
    pathStr[size + 1] = '*';
    pathStr[size + 2] = 0;

    u32 n = 0;

    HANDLE searchHnd = FindFirstFileA(pathStr, &fileList->files[n]);

    if (searchHnd != INVALID_HANDLE_VALUE)
    {
        do
        {
            n++;
        } while (FindNextFileA(searchHnd, &fileList->files[n]) && n < MAX_FILES_LISTABLE);

        fileList->fileCount = n;
    }

    free(pathStr);

    FindClose(searchHnd);
}
