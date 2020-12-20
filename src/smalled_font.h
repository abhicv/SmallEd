#ifndef SMALLED_FONT_H
#define SMALLED_FONT_H

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include "types.h"

#include "smalled_render.h"

typedef struct FontData
{
    stbtt_fontinfo fontInfo;
    
    //data read from font file
    u8 *rawData;
    
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

#endif //SMALLED_FONT_H
