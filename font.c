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
        
        u8 *fontBuffer = malloc(size);
        fread(fontBuffer, 1, size, fontFileHandle);
        
        stbtt_InitFont(&fontInfo, fontBuffer, stbtt_GetFontOffsetForIndex(fontBuffer, 0));
        
        fclose(fontFileHandle);
    }
    
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);
    f32 scale = stbtt_ScaleForPixelHeight(&fontInfo, size);
    
    FontData fontData = {0};
    fontData.fontInfo = fontInfo;
    fontData.size = size;
    fontData.scale = scale;
    fontData.ascent = roundf(ascent * scale);
    fontData.descent = roundf(descent * scale);
    fontData.lineGap = roundf(lineGap * scale);
    
    return fontData;
}

void RenderFontBitMap(Buffer *buffer, u8 *bitMap, Rect *rect)
{
    if(buffer != NULL && bitMap != NULL)
    {
        for(u32 y = rect->y; y < (rect->y + rect->height); y++)
        {
            for(u32 x = rect->x; x < (rect->x + rect->width); x++)
            {
                if(x >= 0 && y >= 0 && x < buffer->width && y < buffer->height)
                {
                    u32 bX = x - rect->x;
                    u32 bY = y - rect->y;
                    u8 alpha = bitMap[bX + bY * rect->width];
                    
                    if(alpha > 50)
                    {
                        buffer->data[x + y * buffer->width] = 
                            (alpha << 32) | 
                            (alpha << 16) | 
                            (alpha << 8) | 
                            (alpha << 0);
                    }
                }
            }
        }
    }
}

void RenderTextBuffer(Buffer *buffer, u8 *textBuffer, FontData *fontData, FontBitMap *fontBitMaps,
                      u32 xPos, u32 yPos,  u32 startIndex)
{
    u32 cursorX = xPos;
    u32 baseline = yPos + fontData->lineGap + fontData->ascent;
    
    u32 i = startIndex;
    char c = textBuffer[i];
    while(c != 0)
    {
        i32 advance;
        i32 lsb;
        stbtt_GetCodepointHMetrics(&fontData->fontInfo, c, &advance, &lsb);
        
        if(c == '\n')
        {
            baseline += fontData->lineGap + fontData->ascent - fontData->descent;
            cursorX = xPos;
        }
        if(c == '\t')
        {
            cursorX += 4 * roundf(advance * fontData->scale);
        }
        if(c == ' ')
        {
            cursorX += roundf(advance * fontData->scale);
        }
        if(c >= '!' && c <= '~')
        {
            Rect glyphRect = {0};
            glyphRect.x = cursorX + fontBitMaps[c - 33].xOffset;
            glyphRect.y = baseline + fontBitMaps[c - 33].yOffset;
            glyphRect.width = fontBitMaps[c - 33].width;
            glyphRect.height = fontBitMaps[c - 33].height;
            
            RenderFontBitMap(buffer, fontBitMaps[c - 33].bitMap, &glyphRect);
            cursorX += roundf(advance * fontData->scale);
            
            if(textBuffer[i + 1])
            {
                i32 kern = stbtt_GetCodepointKernAdvance(&fontData->fontInfo, textBuffer[i], textBuffer[i + 1]);
                cursorX += roundf(kern * fontData->scale);
            }
        }
        
        i++;
        c = textBuffer[i];
    }
}
