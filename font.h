#ifndef FONT_H
#define FONT_H

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include "render.h"
#include "types.h"

typedef struct FontData
{
    stbtt_fontinfo fontInfo;
    u32 size;
    f32 scale;
    
    //stored in pixel unit
    i32 ascent;
    i32 descent;
    i32 lineGap;
    
} FontData;

typedef struct FontBitMap
{
    u8 *bitMap;
    i32 xOffset, yOffset;
    u32 width, height;
    
} FontBitMap;

FontData LoadFont(const char *fontFile, u32 size);
void RenderFontBitMap(Buffer *buffer, u8 *bitMap, Rect *rect);
void RenderTextBuffer(Buffer *buffer, u8 *textBuffer, FontData *fontData, FontBitMap *fontBitMaps, 
                      u32 xPos, u32 yPos,  u32 startIndex);
#endif //FONT_H
