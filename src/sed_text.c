#include "sed_text.h"

//text sequence
void InsertItem(TextSequence *tSeq, u8 character)
{
    if(tSeq->gapSize > 0)
    {
        tSeq->buffer[tSeq->preSize] = character;
        tSeq->preSize++;
        tSeq->gapSize--;
    }
}

void DeleteItem(TextSequence *tSeq)
{
    if(tSeq->preSize > 0)
    {
        tSeq->preSize--;
        tSeq->gapSize++;
    }
}

void MoveCursorLeftLocal(TextSequence *tSeq)
{
    if(tSeq->preSize > 0)
    {
        u32 fromIndex = tSeq->preSize - 1;
        u32 toIndex = tSeq->bufferCapacity - tSeq->postSize - 1;
        
        tSeq->buffer[toIndex] = tSeq->buffer[fromIndex];
        
        tSeq->preSize--;
        tSeq->postSize++;
    }
}

void MoveCursorRightLocal(TextSequence *tSeq)
{
    if(tSeq->postSize > 0)
    {
        u32 fromIndex = tSeq->bufferCapacity - tSeq->postSize;
        u32 toIndex = tSeq->preSize;
        
        tSeq->buffer[toIndex] = tSeq->buffer[fromIndex];
        
        tSeq->preSize++;
        tSeq->postSize--;
    }
}

void MoveCursorToLeftPosition(TextSequence *tSeq, u32 position)
{
    while(tSeq->preSize > position)
    {
        MoveCursorLeftLocal(tSeq);
    }
}

void MoveCursorToRightPosition(TextSequence *tSeq, u32 position)
{
    while(tSeq->preSize < position)
    {
        MoveCursorRightLocal(tSeq);
    }
}

void MoveCursorToPosition(TextSequence *tSeq, u32 position)
{
    position = (position > (tSeq->preSize + tSeq->postSize)) ? (tSeq->preSize + tSeq->postSize) : position;
    
    if(tSeq->preSize < position)
    {
        MoveCursorToRightPosition(tSeq, position);
    }
    else if(tSeq->preSize > position)
    {
        MoveCursorToLeftPosition(tSeq, position);
    }
}

#define FIXED_LINE_SIZE_IN_BYTES 2048

//text buffer
//loading text file as lines into the text buffer
void LoadTextBuffer(u8 *fileBuffer, u32 fileSize, LineInfo *lineInfos, u32 nLines, TextBuffer *textBuffer)
{
    u32 memUsage = 0;
    
    for(u32 n = 0; n < nLines; n++)
    {
        u32 lineStartIndex = lineInfos[n].lineStartIndex;
        u32 lineSize = lineInfos[n].lineSize;
        
        if(n == 0)
        {
            if(lineSize > FIXED_LINE_SIZE_IN_BYTES)
            {
                printf("File line size more than %d Kb\n", FIXED_LINE_SIZE_IN_BYTES);
                textBuffer->loaded = false;
                return;
            }
            
            TextSequence *tSeq = &textBuffer->lines[textBuffer->preSize];
            
            tSeq->bufferCapacity = FIXED_LINE_SIZE_IN_BYTES;
            
            //tSeq->buffer = (u8*)AllocatePersistentMemory(tSeq->bufferCapacity);
            //tSeq->colorIndexBuffer = (u8*)AllocatePersistentMemory(tSeq->bufferCapacity);
            tSeq->preSize = 0;
            tSeq->postSize = lineSize;
            tSeq->gapSize = tSeq->bufferCapacity - tSeq->postSize;
            
            memcpy(&tSeq->buffer[tSeq->bufferCapacity - tSeq->postSize], &fileBuffer[lineStartIndex], lineSize);
            
            textBuffer->preSize++;
            
            memUsage += tSeq->bufferCapacity;
        }
        else
        {
            if(lineSize > FIXED_LINE_SIZE_IN_BYTES)
            {
                textBuffer->loaded = false;
                printf("File line size more than %d Kb\n", FIXED_LINE_SIZE_IN_BYTES);
                return;
            }
            
            TextSequence *tSeq = &textBuffer->lines[textBuffer->capacity - textBuffer->postSize + n - 1];
            
            tSeq->bufferCapacity = FIXED_LINE_SIZE_IN_BYTES;
            
            //tSeq->buffer = (u8*)AllocatePersistentMemory(tSeq->bufferCapacity);
            //tSeq->colorIndexBuffer = (u8*)AllocatePersistentMemory(tSeq->bufferCapacity);
            tSeq->preSize = 0;
            tSeq->postSize = lineSize;
            tSeq->gapSize = tSeq->bufferCapacity - tSeq->postSize;
            
            memcpy(&tSeq->buffer[tSeq->bufferCapacity - tSeq->postSize], &fileBuffer[lineStartIndex], lineSize);
            
            memUsage += tSeq->bufferCapacity;
        }
    }
    
    printf("Textbuffer memory usage: %d bytes\n", memUsage);
    printf("Original file size: %d bytes\n", fileSize - 1);
    
    textBuffer->loaded = true;
}

void InsertLine(TextBuffer *textBuffer)
{
    if(textBuffer->gapSize > 0)
    {
        TextSequence *tSeq = &textBuffer->lines[textBuffer->preSize];
        
        //tSeq->bufferCapacity = FIXED_LINE_SIZE_IN_BYTES;
        
        tSeq->preSize = 0;
        tSeq->postSize = textBuffer->lines[textBuffer->currentLine].postSize;
        tSeq->gapSize = tSeq->bufferCapacity - tSeq->postSize;
        
        if(tSeq->postSize > 0)
        {
            memcpy(&tSeq->buffer[tSeq->bufferCapacity - tSeq->postSize], 
                   &textBuffer->lines[textBuffer->currentLine].buffer[textBuffer->lines[textBuffer->currentLine].bufferCapacity - tSeq->postSize], 
                   tSeq->postSize);
        }
        
        //shrinking previous line
        textBuffer->lines[textBuffer->currentLine].postSize = 0;
        
        textBuffer->preSize++;
        textBuffer->gapSize--;
        textBuffer->currentLine++;
    }
}

void DeleteLine(TextBuffer *textBuffer)
{
    if(textBuffer->preSize > 1)
    {
        TextSequence *cTSeq = &textBuffer->lines[textBuffer->currentLine];
        TextSequence *tSeq = &textBuffer->lines[textBuffer->currentLine - 1];
        
        while(tSeq->postSize > 0)
        {
            MoveCursorRightLocal(tSeq);
        }
        
        if(cTSeq->postSize > 0)
        {
            for(u32 n = (cTSeq->bufferCapacity - cTSeq->postSize); n < cTSeq->bufferCapacity; n++)
            {
                InsertItem(tSeq, cTSeq->buffer[n]);
            }
        }
        
        for(u32 n = 0; n < (cTSeq->preSize + cTSeq->postSize); n++)
        {
            MoveCursorLeftLocal(tSeq);
        }
        
        textBuffer->preSize--;
        textBuffer->currentLine--;
        textBuffer->gapSize++;
    }
}

void MoveCursorUp(TextBuffer *textBuffer)
{
    if(textBuffer->preSize > 1)
    {
        u32 fromIndex = textBuffer->preSize - 1;
        u32 toIndex = textBuffer->capacity - textBuffer->postSize - 1;
        
        TextSequence tmp = textBuffer->lines[toIndex];
        textBuffer->lines[toIndex] = textBuffer->lines[fromIndex];
        textBuffer->lines[fromIndex] = tmp;
        
        textBuffer->preSize--;
        textBuffer->postSize++;
        textBuffer->currentLine--;
        
        MoveCursorToPosition(&textBuffer->lines[textBuffer->currentLine], textBuffer->desiredCursorPos);
    }
}

void MoveCursorDown(TextBuffer *textBuffer)
{
    if(textBuffer->postSize > 0)
    {
        u32 fromIndex = textBuffer->capacity - textBuffer->postSize;
        u32 toIndex = textBuffer->preSize;
        
        TextSequence tmp = textBuffer->lines[toIndex];
        textBuffer->lines[toIndex] = textBuffer->lines[fromIndex];
        textBuffer->lines[fromIndex] = tmp;
        
        textBuffer->preSize++;
        textBuffer->postSize--;
        textBuffer->currentLine++;
        
        MoveCursorToPosition(&textBuffer->lines[textBuffer->currentLine], textBuffer->desiredCursorPos);
    }
}

void MoveCursorRight(TextBuffer *textBuffer)
{
    if(textBuffer->lines[textBuffer->currentLine].postSize > 0)
    {
        MoveCursorRightLocal(&textBuffer->lines[textBuffer->currentLine]);
    }
    else
    {
        if(textBuffer->postSize > 0)
        {
            MoveCursorDown(textBuffer);
            
            while(textBuffer->lines[textBuffer->currentLine].preSize > 0)
            {
                MoveCursorLeftLocal(&textBuffer->lines[textBuffer->currentLine]);
            }
        }
    }
    textBuffer->desiredCursorPos = textBuffer->lines[textBuffer->currentLine].preSize;
}

void MoveCursorLeft(TextBuffer *textBuffer)
{
    if(textBuffer->lines[textBuffer->currentLine].preSize > 0)
    {
        MoveCursorLeftLocal(&textBuffer->lines[textBuffer->currentLine]);
    }
    else
    {
        if(textBuffer->preSize > 1)
        {
            MoveCursorUp(textBuffer);
            
            while(textBuffer->lines[textBuffer->currentLine].postSize > 0)
            {
                MoveCursorRightLocal(&textBuffer->lines[textBuffer->currentLine]);
            }
        }
    }
    
    textBuffer->desiredCursorPos = textBuffer->lines[textBuffer->currentLine].preSize;
}

void GotoLine(TextBuffer *textBuffer, u32 lineNum)
{
    u32 totalLineCount = (textBuffer->preSize + textBuffer->postSize);
    
    lineNum = lineNum > totalLineCount ? totalLineCount : lineNum;
    lineNum = lineNum > 0 ? lineNum : 1;
    
    if(lineNum > (textBuffer->currentLine + 1))
    {
        while(textBuffer->preSize < lineNum)
        {
            MoveCursorDown(textBuffer);
        }
    }
    else
    {
        while(textBuffer->preSize > lineNum)
        {
            MoveCursorUp(textBuffer);
        }
    }
    
    u32 half = (textBuffer->maxLinesVisible / 2);
    
    if(lineNum < half)
    {
        textBuffer->lowestLine = 0;
    }
    else
    {
        textBuffer->lowestLine = lineNum - half;
    }
}

u32 GetTextBufferSize(TextBuffer *textBuffer)
{
    u32 size = 0;
    
    for(u32 n = 0; n < textBuffer->preSize; n++)
    {
        size += (textBuffer->lines[n].preSize + textBuffer->lines[n].postSize);
    }
    
    for(u32 n = textBuffer->capacity - textBuffer->postSize; n < textBuffer->capacity; n++)
    {
        size += (textBuffer->lines[n].preSize + textBuffer->lines[n].postSize);
    }
    
    size += textBuffer->preSize + textBuffer->postSize - 1;
    
    return size;
}

void TextBufferToPlainBuffer(TextBuffer *textBuffer, u8 *buffer)
{
    u32 offset = 0;
    u32 lineCount = textBuffer->preSize + textBuffer->postSize;
    
    if(buffer != NULL)
    {
        u32 n = 0;
        for(n = 0; n < textBuffer->preSize; n++)
        {
            if(textBuffer->lines[n].preSize > 0)
            {
                memcpy(buffer + offset, textBuffer->lines[n].buffer, textBuffer->lines[n].preSize);
                offset += textBuffer->lines[n].preSize;
            }
            
            if(textBuffer->lines[n].postSize > 0)
            {
                memcpy(buffer + offset, textBuffer->lines[n].buffer + textBuffer->lines[n].bufferCapacity - textBuffer->lines[n].postSize, textBuffer->lines[n].postSize);
                offset += textBuffer->lines[n].postSize;
            }
            
            if(n != (lineCount - 1))
            {
                buffer[offset] = '\n';
                offset++;
            }
        }
        
        if(textBuffer->postSize > 0)
        {
            for(n = (textBuffer->capacity - textBuffer->postSize); n < textBuffer->capacity; n++)
            {
                if(textBuffer->lines[n].preSize > 0)
                {
                    memcpy(buffer + offset, textBuffer->lines[n].buffer, textBuffer->lines[n].preSize);
                    offset += textBuffer->lines[n].preSize;
                }
                
                if(textBuffer->lines[n].postSize > 0)
                {
                    memcpy(buffer + offset, textBuffer->lines[n].buffer + textBuffer->lines[n].bufferCapacity - textBuffer->lines[n].postSize, textBuffer->lines[n].postSize);
                    offset += textBuffer->lines[n].postSize;
                }
                
                if(n != (textBuffer->capacity - 1))
                {
                    buffer[offset] = '\n';
                    offset++;
                }
            }
        }
    }
    
    return;
}

//Process file buffer at load time to get line information and line count
LineInfo* PreprocessFileBuffer(u8 *buffer, u32 bufferSize, u32 *nLines)
{
    u32 lineCount = 0;
    u32 lineSize = 0;
    u32 lineStartIndex = 0;
    
    LineInfo *lineInfos = 0;
    
    for(u32 n = 0; n < bufferSize; n++)
    {
        if(buffer[n] == '\n' || buffer[n] == 0)
        {
            if(lineCount == 0)
            {
                lineInfos = (LineInfo*)malloc(sizeof(LineInfo));
                lineInfos[0].lineStartIndex = lineStartIndex;
                lineInfos[0].lineSize = lineSize;
            }
            else
            {
                lineInfos = (LineInfo*)realloc(lineInfos, sizeof(LineInfo) * (lineCount+1));
                lineInfos[lineCount].lineStartIndex = lineStartIndex;
                lineInfos[lineCount].lineSize = lineSize;
            }
            
            lineSize = 0;
            lineCount++;
            lineStartIndex = n + 1;
        }
        else
        {
            lineSize++;
        }
    }
    
    *nLines = lineCount;
    return lineInfos;
}



