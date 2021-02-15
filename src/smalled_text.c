#include "smalled_text.h"

//text sequence
void InsertItem(TextSequence *tSeq, u8 character)
{
    if(tSeq->gapSize > 0)
    {
        tSeq->buffer[tSeq->preSize] = character;
        tSeq->preSize++;
        tSeq->gapSize--;
    }
    else
    {
        //TODO(abhicv): resize text buffer to create new gap
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

void SeekCursorLeft(TextSequence *tSeq, u32 steps)
{
    
}

void SeekCursorRight(TextSequence *tSeq, u32 steps)
{
    
}

#define MAX_LINE_SIZE_KB 100 

//text buffer
//loading text file as lines into the text buffer 
void BreakFileIntoLines(u8 *fileBuffer, u32 fileSize, u32 nLines, TextBuffer *textBuffer)
{
    u32 nCharInLine = 0;
    u32 lineStartIndex = 0;
    u32 lineCount = 0;
    
    for(u32 n = 0; n < fileSize; n++)
    {
        if((fileBuffer[n] == '\n') || (n == fileSize - 1))
        {
            if(lineCount == 0)
            {
                TextSequence *tSeq = &textBuffer->lines[textBuffer->preSize];
                
                tSeq->bufferCapacity = KiloByte(MAX_LINE_SIZE_KB);
                tSeq->buffer = (u8*)malloc(tSeq->bufferCapacity);
                tSeq->colorIndexBuffer = (u8*)malloc(tSeq->bufferCapacity);
                tSeq->preSize = 0;
                tSeq->postSize = nCharInLine;
                tSeq->gapSize = tSeq->bufferCapacity - tSeq->postSize;
                
                memcpy(&tSeq->buffer[tSeq->bufferCapacity - tSeq->postSize], &fileBuffer[lineStartIndex], nCharInLine);
                
                textBuffer->preSize++;
            }
            else
            {
                TextSequence *tSeq = &textBuffer->lines[textBuffer->capacity - textBuffer->postSize + lineCount - 1];
                
                if(n == fileSize - 1)
                {
                    nCharInLine = 1;
                }
                
                tSeq->bufferCapacity = KiloByte(MAX_LINE_SIZE_KB);
                tSeq->buffer = (u8*)malloc(tSeq->bufferCapacity);
                tSeq->colorIndexBuffer = (u8*)malloc(tSeq->bufferCapacity);
                tSeq->preSize = 0;
                tSeq->postSize = nCharInLine;
                tSeq->gapSize = tSeq->bufferCapacity - tSeq->postSize;
                
                memcpy(&tSeq->buffer[tSeq->bufferCapacity - tSeq->postSize], &fileBuffer[lineStartIndex], nCharInLine);
            }
            
            nCharInLine = 0;
            lineStartIndex = n + 1;
            lineCount++;
        }
        else
        {
            nCharInLine++;
        }
    }
}

void InsertLine(TextBuffer *textBuffer)
{
    TextSequence *tSeq = &textBuffer->lines[textBuffer->preSize];
    
    tSeq->bufferCapacity = KiloByte(2);
    
    tSeq->buffer = (u8*)malloc(tSeq->bufferCapacity);
    tSeq->colorIndexBuffer = (u8*)malloc(tSeq->bufferCapacity);
    
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
        
        for(u32 n = 0; n < cTSeq->preSize; n++)
        {
            InsertItem(tSeq, cTSeq->buffer[n]);
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
        
        free(cTSeq->buffer);
        free(cTSeq->colorIndexBuffer);
        
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
        
        textBuffer->lines[toIndex] = textBuffer->lines[fromIndex];
        
        textBuffer->preSize--;
        textBuffer->postSize++;
        textBuffer->currentLine--;
    }
}

void MoveCursorDown(TextBuffer *textBuffer)
{
    if(textBuffer->postSize > 0)
    {
        u32 fromIndex = textBuffer->capacity - textBuffer->postSize;
        u32 toIndex = textBuffer->preSize;
        
        textBuffer->lines[toIndex] = textBuffer->lines[fromIndex];
        
        textBuffer->preSize++;
        textBuffer->postSize--;
        textBuffer->currentLine++;
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
        MoveCursorDown(textBuffer);
        
        while(textBuffer->lines[textBuffer->currentLine].preSize > 0)
        {
            MoveCursorLeftLocal(&textBuffer->lines[textBuffer->currentLine]);
        }
    }
}

void MoveCursorLeft(TextBuffer *textBuffer)
{
    if(textBuffer->lines[textBuffer->currentLine].preSize > 0)
    {
        MoveCursorLeftLocal(&textBuffer->lines[textBuffer->currentLine]);
    }
    else if(textBuffer->preSize > 1)
    {
        MoveCursorUp(textBuffer);
        
        while(textBuffer->lines[textBuffer->currentLine].postSize > 0)
        {
            MoveCursorRightLocal(&textBuffer->lines[textBuffer->currentLine]);
        }
    }
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

