#ifndef SED_EDITOR_H
#define SED_EDITOR_H

#include <SDL2/SDL.h>

#include "sed_render.h"
#include "sed_font.h"
#include "sed_text.h"
#include "sed_types.h"

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

#endif //SED_EDITOR_H
