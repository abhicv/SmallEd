#include "text.h"

void InsertItem(TextSequence *tSeq, u8 item)
{
    if(tSeq->gapSize > 0)
    {
        tSeq->buffer[++tSeq->preEndIndex] = item;
        tSeq->gapSize--;
    }
    else
    {
        //TODO(abhicv): resize text buffer to create new gap
    }
}

void DeleteItem(TextSequence *tSeq)
{
    if(tSeq->preEndIndex > -1)
    {
        tSeq->preEndIndex--;
        tSeq->gapSize++;
    }
}

void MoveCursorLeft(TextSequence *tSeq)
{
    if(tSeq->preEndIndex > -1)
    {
        tSeq->buffer[--tSeq->postStartIndex] = tSeq->buffer[tSeq->preEndIndex--];
    }
}

void MoveCursorRight(TextSequence *tSeq)
{
    if(tSeq->postStartIndex < TEXT_BUFFER_SIZE)
    {
        tSeq->buffer[++tSeq->preEndIndex] = tSeq->buffer[tSeq->postStartIndex++];
    }
}

void MoveCursorUp(TextSequence *tSeq)
{
    u32 currentLineCharCount = 0;
    while(tSeq->buffer[tSeq->preEndIndex] != '\n' && tSeq->preEndIndex > -1)
    {
        MoveCursorLeft(tSeq);
        currentLineCharCount++;
    }
    
    MoveCursorLeft(tSeq);
    
    u32 upperLineCharCount = 0;
    while(tSeq->buffer[tSeq->preEndIndex] != '\n' && tSeq->preEndIndex > -1)
    {
        MoveCursorLeft(tSeq);
        upperLineCharCount++;
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

void MoveCursorDown(TextSequence *tSeq)
{
    u32 currentLineCharCount = 0;
    
    while(tSeq->buffer[tSeq->preEndIndex] != '\n' && tSeq->preEndIndex > -1)
    {
        MoveCursorLeft(tSeq);
        currentLineCharCount++;
    }
    
    while(tSeq->buffer[tSeq->postStartIndex] != '\n' && tSeq->postStartIndex < TEXT_BUFFER_SIZE)
    {
        MoveCursorRight(tSeq);
    }
    
    MoveCursorRight(tSeq);
    
    u32 n = 0;
    while(n < currentLineCharCount && tSeq->buffer[tSeq->postStartIndex] != '\n' && tSeq->postStartIndex < TEXT_BUFFER_SIZE)
    {
        MoveCursorRight(tSeq);
        n++;
    }
}
