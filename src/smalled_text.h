#ifndef SMALLED_TEXT_H
#define SMALLED_TEXT_H

#include "types.h"
#include "smalled_memory.h"

//Gap buffer of u8(ascii character)
typedef struct TextSequence
{
    u8 *buffer;
    
    u32 preSize;
    u32 postSize;
    u32 gapSize;
    
    u32 bufferCapacity;
    
    //for syntax highlighting
    u8* colorIndexBuffer;
    
} TextSequence;

//Gap buffer of TextSequence
typedef struct TextBuffer
{
    TextSequence *lines;
    
    u32 preSize;
    u32 postSize;
    u32 gapSize;
    u32 capacity;
    
    u32 currentLine;
    u32 lowestLine;
    u32 maxLinesVisible;
    
} TextBuffer;

//text sequence
void InsertItem(TextSequence *tSeq, u8 character);
void DeleteItem(TextSequence *tSeq);

void MoveCursorLeftLocal(TextSequence *tSeq);
void MoveCursorRightLocal(TextSequence *tSeq);

void BreakFileIntoLines(u8 *fileBuffer, u32 fileSize, u32 nLines, TextBuffer *textBuffer);

//text buffer 
void InsertLine(TextBuffer *textBuffer);
void DeleteLine(TextBuffer *textBuffer);
void MoveCursorUp(TextBuffer *textBuffer);
void MoveCursorDown(TextBuffer *textBuffer);
void MoveCursorRight(TextBuffer *textBuffer);
void MoveCursorLeft(TextBuffer *textBuffer);
void GotoLine(TextBuffer *textBuffer, u32 lineNum);

#endif //SMALLED_TEXT_H
