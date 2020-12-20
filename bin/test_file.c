#include "font.h"

FontData LoadFont(const char *fontFile, u32 size)
{
    stbtt_fontinfo fontInfo = {0};
    FILE *fontFileHandle = fopen(fontFile, "rb");
    if(fontFileHandle != NULL)
    {
        fseek(fontFileHandle, 0, SEEK_END);
        u32 size = ftell(fontFileHandle);
        fseek(fontFileHandle, 0, SEEK_SET);
        
        u8 *fontBuffer = (u8*)malloc(size);
        fread(fontBuffer, 1, size, fontFileHandle);
        
        stbtt_InitFont(&fontInfo, fontBuffer, stbtt_GetFontOffsetForIndex(fontBuffer, 0));
        
        fclose(fontFileHandle);
    }
    FontData fontData = {0};
    fontData.fontInfo = fontInfo;
    fontData.size = size;
    fontData.scale = scale;
    fontData.ascent = roundf(ascent * scale);
    fontData.descent = roundf(descent * scale);
    fontData.lineGap = roundf(lineGap * scale);
    
    return fontData;
}

Color BlendPixel(Color dst, Color src, Color color) 
{
    src.a = (src.a * color.a) >> 8;
    int ia = 0xff - src.a;
    
    dst.r = ((src.r * color.r * src.a) >> 16) + ((dst.r * ia) >> 8);
    dst.g = ((src.g * color.g * src.a) >> 16) + ((dst.g * ia) >> 8);
    dst.b = ((src.b * color.b * src.a) >> 16) + ((dst.b * ia) >> 8);
    
    return dst;
}
