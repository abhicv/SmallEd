#ifndef SED_TEXT_H
#define SED_TEXT_H

#include "sed_types.h"
#include "sed_memory.h"

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
    
    b32 rlMoved; //check for left right cursor movement
    u32 lastPreSize;
    
    b32 loaded;
} TextBuffer;

typedef struct LineInfo
{
    u32 lineStartIndex;
    u32 lineSize; //num of characters in a line
    
} LineInfo;

//text sequence
void InsertItem(TextSequence *tSeq, u8 character);
void DeleteItem(TextSequence *tSeq);

void MoveCursorLeftLocal(TextSequence *tSeq);
void MoveCursorRightLocal(TextSequence *tSeq);

//text buffer 
void InsertLine(TextBuffer *textBuffer);
void DeleteLine(TextBuffer *textBuffer);
void MoveCursorUp(TextBuffer *textBuffer);
void MoveCursorDown(TextBuffer *textBuffer);
void MoveCursorRight(TextBuffer *textBuffer);
void MoveCursorLeft(TextBuffer *textBuffer);
void GotoLine(TextBuffer *textBuffer, u32 lineNum);

#endif //SED_TEXT_H
