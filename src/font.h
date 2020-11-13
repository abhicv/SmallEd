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

#endif //FONT_H
