#include "smalled_text.h"

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

void MoveCursorLeft(TextSequence *tSeq)
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

void MoveCursorRight(TextSequence *tSeq)
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

void MoveCursorUp(TextSequence *tSeq)
{
    u32 currentLineCharCount = 0;
    
    if(tSeq->preSize > 0)
    {
        while(tSeq->buffer[tSeq->preSize - 1] != '\n')
        {
            MoveCursorLeft(tSeq);
            
            if(tSeq->preSize == 0)
            {
                return;
            }
            
            currentLineCharCount++;
        }
        
        MoveCursorLeft(tSeq);
        
        u32 upperLineCharCount = 0;
        while(tSeq->buffer[tSeq->preSize - 1] != '\n')
        {
            MoveCursorLeft(tSeq);
            upperLineCharCount++;
            
            if(tSeq->preSize == 0)
            {
                break;
            }
        }
        
        if(upperLineCharCount >= currentLineCharCount)
        {
            for(u32 n = 0; n < currentLineCharCount; n++)
            {
                MoveCursorRight(tSeq);
            }
        }
        else
        {
            for(u32 n = 0; n < upperLineCharCount; n++)
            {
                MoveCursorRight(tSeq);
            }
        }
    }
}

void MoveCursorDown(TextSequence *tSeq)
{
    u32 currentLineCharCount = 0;
    
    if(tSeq->preSize > 0)
    {
        while(tSeq->buffer[tSeq->preSize - 1] != '\n')
        {
            MoveCursorLeft(tSeq);
            currentLineCharCount++;
            
            if(tSeq->preSize == 0)
            {
                break;
            }
        }
    }
    
    if(tSeq->postSize > 0)
    {
        while(tSeq->buffer[tSeq->bufferCapacity - tSeq->postSize] != '\n')
        {
            MoveCursorRight(tSeq);
            
            if(tSeq->postSize == 0)
            {
                return;
            }
        }
    }
    
    MoveCursorRight(tSeq);
    
    if(tSeq->postSize > 0)
    {
        u32 n = 0;
        while(n < currentLineCharCount && tSeq->buffer[tSeq->bufferCapacity - tSeq->postSize] != '\n')
        {
            MoveCursorRight(tSeq);
            n++;
            
            if(tSeq->postSize == 0)
            {
                break;
            }
        }
    }
}