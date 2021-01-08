#ifndef SMALLED_FONT_H
#define SMALLED_FONT_H

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include "types.h"

#include "smalled_render.h"
#include "smalled_debug.h"
#include "smalled_lexer.h"
#include "smalled_text.h"
#include "smalled_memory.h"

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

FontData* LoadFont(Memory *memory, const u8 *fontFileName, f32 fontSize);

Color BlendPixel(Color dst, Color src, Color color);
Color GetBufferPixelColor(Buffer *buffer, u32 x, u32 y);
void RenderFontBitMap(Buffer *renderBuffer, Rect *destRect, BitMap *atlasBitMap, Rect *srcRect, Color color);
u32 RenderText(Buffer *renderBuffer, u8 *textBuffer, u32 size, FontData *fontData, u32 xPos, u32 yPos, Color color);

#endif //SMALLED_FONT_H
