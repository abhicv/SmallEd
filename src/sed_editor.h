#ifndef SED_EDITOR_H
#define SED_EDITOR_H

#include <SDL2/SDL.h>

#include "sed_file.h"
#include "sed_font.h"
#include "sed_piece_table.h"
#include "sed_render.h"
#include "sed_types.h"
#include "sed_util.h"

typedef struct Editor
{
    PieceTable pieceTable;
    Rect rect;
    u8 *fileName;
    u32 mode;
    FileList fileList;
} Editor;

typedef enum EditorMode
{
    EDITOR_MODE_GOTO_LINE = 0,
    EDITOR_MODE_FILE_LIST,
    EDITOR_MODE_EDIT,
    EDITOR_MODE_ENTRY,
} EditorMode;

#endif // SED_EDITOR_H
