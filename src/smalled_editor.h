#ifndef SMALLED_EDITOR_H
#define SMALLED_EDITOR_H

#include <SDL2/SDL.h>

#include "smalled_render.h"
#include "smalled_text.h"

#include "types.h"

typedef struct Editor
{
    TextBuffer textBuffer;
    Rect rect;
    
    u8 *fileName;
    b32 gotoLine;
    TextSequence gotoLineTSeq;
    
} Editor;

u32 DigitCount(u32 n);
void EditorSpaceEvent(SDL_Event *event, Editor *editor);
void RenderEditor(Buffer *renderBuffer, FontData *fontData, Editor *editor);

#endif //SMALLED_EDITOR_H
