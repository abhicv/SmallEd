#ifndef TEXT_H
#define TEXT_H

typedef struct TextSequence
{
    u8 *buffer;
    i32 preEndIndex;
    i32 postStartIndex;
    u32 gapSize;
    u32 bufferSize;
    
} TextSequence;

void InsertItem(TextSequence *tSeq, u8 item);
void DeleteItem(TextSequence *tSeq);

//cursor movement
void MoveLeft(TextSequence *tSeq);
void MoveRight(TextSequence *tSeq);
void MoveUp(TextSequence *tSeq);
void MoveDown(TextSequence *tSeq);

#define TEXT_BUFFER_SIZE 1024 * 1024 //1 megabyte of text edit buffer(gap buffer)

#endif //TEXT_H
