#ifndef SMALLED_EDITOR_H
#define SMALLED_EDITOR_H

#include <SDL2/SDL.h>

#include "smalled_render.h"
#include "smalled_font.h"
#include "smalled_text.h"

#include "types.h"

typedef struct Editor
{
    TextBuffer textBuffer;
    Rect rect;
    
    u8 *fileName;
    
    TextSequence gotoLineTSeq;
    
    u32 mode;
    
    FileList fileList;
    
} Editor;

enum EditorMode
{
    EDITOR_MODE_GOTO_LINE,
    EDITOR_MODE_FILE_LIST,
    EDITOR_MODE_EDIT,
    EDITOR_MODE_ENTRY,
};

#endif //SMALLED_EDITOR_H
