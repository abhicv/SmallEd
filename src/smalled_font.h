#ifndef SMALLED_FONT_H
#define SMALLED_FONT_H

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include "types.h"

#include "smalled_render.h"

typedef struct BitMap
{
    u8 *pixels;
    u32 width; 
    u32 height;
    
} BitMap;

typedef struct FontData
{
    stbtt_fontinfo fontInfo;
    stbtt_bakedchar charDatas[256];
    
    //raw data read from ttf file
    u8 *rawFontData;
    
    BitMap atlasBitMap;
    
    f32 fontSize;
    u32 lineHeight;
    
    //stored in pixel unit
    
} FontData;

typedef struct FontBitMap
{
    u8 *bitMap;
    i32 xOffset, yOffset;
    u32 width, height;
    
} FontBitMap;

#endif //SMALLED_FONT_H
