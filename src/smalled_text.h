#ifndef SMALLED_TEXT_H
#define SMALLED_TEXT_H

typedef struct TextSequence
{
    u8 *buffer;
    u32 preSize;
    u32 postSize;
    u32 gapSize;
    u32 bufferCapacity;
    
    u8* colorIndexBuffer;
    
} TextSequence;

void InsertItem(TextSequence *tSeq, u8 item);
void DeleteItem(TextSequence *tSeq);

//cursor movement
void MoveCursorLeft(TextSequence *tSeq);
void MoveCursorRight(TextSequence *tSeq);
void MoveCursorUp(TextSequence *tSeq);
void MoveCursorDown(TextSequence *tSeq);

#define TEXT_BUFFER_SIZE 10 * 1024 * 1024 //1 megabyte of text edit buffer(gap buffer)

#endif //SMALLED_TEXT_H
