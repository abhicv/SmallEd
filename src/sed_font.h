#ifndef SED_FONT_H
#define SED_FONT_H

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#include "sed_types.h"
#include "sed_render.h"
#include "sed_debug.h"
#include "sed_lexer.h"
#include "sed_text.h"
#include "sed_memory.h"

#define RENDERABLE_CHAR(c) ((c >= 32) && (c <= 126)) 

//8-bit single channel bitmap
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
    
    //raw font data read from ttf file
    u8 *rawFontData;
    
    BitMap atlasBitMap;
    
    f32 fontSize;
    u32 lineHeight;
    
} FontData;

#endif //SED_FONT_H
