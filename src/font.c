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
    
    i32 ascent, descent, lineGap;
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

Color BlendPixel(Color dst, Color src, Color color) 
{
    src.a = (src.a * color.a) >> 8;
    i32 ia = 0xff - src.a;
    
    dst.r = ((src.r * color.r * src.a) >> 16) + ((dst.r * ia) >> 8);
    dst.g = ((src.g * color.g * src.a) >> 16) + ((dst.g * ia) >> 8);
    dst.b = ((src.b * color.b * src.a) >> 16) + ((dst.b * ia) >> 8);
    
    return dst;
}

Color BlendPix(Color dst, Color src) 
{
    int ia = 0xff - src.a;
    
    dst.r = ((src.r * src.a) + (dst.r * ia)) >> 8;
    dst.g = ((src.g * src.a) + (dst.g * ia)) >> 8;
    dst.b = ((src.b * src.a) + (dst.b * ia)) >> 8;
    
    return dst;
}

Color GetBufferPixelColor(Buffer *buffer, u32 x, u32 y)
{
    Color color = {0};
    
    color.r = (buffer->data[x + y * buffer->width] & 0xFF000000) >> 24;
    color.g = (buffer->data[x + y * buffer->width] & 0x00FF0000) >> 16;
    color.b = (buffer->data[x + y * buffer->width] & 0x0000FF00) >> 8;
    color.a = (buffer->data[x + y * buffer->width] & 0x000000FF) >> 0;
    
    return color;
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
                    
                    //if(alpha > 0)
                    {
#if 0
                        buffer->data[x + y * buffer->width] = 
                            (alpha << 24) | 
                            (alpha << 16) | 
                            (alpha << 8) | 
                            (alpha  << 0);
#endif
                        
#if 1
                        Color dst = GetBufferPixelColor(buffer, x, y);
                        Color src = {alpha, alpha, alpha, alpha};
                        Color c = {255, 255, 255, 255};
                        Color blendColor = BlendPixel(dst, src, c);
                        
                        buffer->data[x + y * buffer->width] = 
                            (blendColor.r << 24) | 
                            (blendColor.g << 16) | 
                            (blendColor.b << 8) | 
                            (blendColor.a  << 0);
#endif 
                    }
                }
            }
        }
    }
}

void RenderText(Buffer *buffer, u8 *textBuffer, FontData *fontData, FontBitMap *fontBitMaps, u32 xPos, u32 yPos, 
                u32 startIndex, u32 endIndex)
{
    u32 cursorX = xPos;
    u32 baseline = yPos + fontData->ascent + fontData->lineGap;
    
    for(u32 i = startIndex; i <= endIndex; i++)
    {
        char c = textBuffer[i];
        
        i32 advance = 0, lsb = 0;
        stbtt_GetCodepointHMetrics(&fontData->fontInfo, c, &advance, &lsb);
        
        if(c == '\n') //new line
        {
            baseline += fontData->lineGap + fontData->ascent - fontData->descent;
            cursorX = xPos;
        }
        if(c == '\t') //tab
        {
            cursorX += 4 * roundf(advance * fontData->scale);
        }
        if(c == ' ') //space
        {
            cursorX += roundf(advance * fontData->scale);
        }
        if(c >= '!' && c <= '~')
        {
            Rect glyphRect = {0};
            glyphRect.x = cursorX + fontBitMaps[c].xOffset;
            glyphRect.y = baseline + fontBitMaps[c].yOffset;
            glyphRect.width = fontBitMaps[c].width;
            glyphRect.height = fontBitMaps[c].height;
            
            RenderFontBitMap(buffer, fontBitMaps[c].bitMap, &glyphRect);
            
            cursorX += roundf(advance * fontData->scale);
            
            if(textBuffer[i + 1])
            {
                i32 kern = stbtt_GetCodepointKernAdvance(&fontData->fontInfo, textBuffer[i], textBuffer[i + 1]);
                cursorX += roundf(kern * fontData->scale);
            }
        }
    }
}

void RenderTextBuffer(Buffer *buffer, u8 *textBuffer, FontData *fontData, FontBitMap *fontBitMaps, u32 xPos, u32 yPos, 
                      i32 preEndIndex, i32 postStartIndex,
                      u32 startIndex, u32 endIndex)
{
    u32 cursorX = xPos;
    u32 baseline = yPos + fontData->ascent + fontData->lineGap;
    
    for(i32 i = startIndex; i <= preEndIndex; i++)
    {
        char c = textBuffer[i];
        
        i32 advance = 0, lsb = 0;
        stbtt_GetCodepointHMetrics(&fontData->fontInfo, c, &advance, &lsb);
        
        if(c == '\n') //new line
        {
            baseline += fontData->lineGap + fontData->ascent - fontData->descent;
            cursorX = xPos;
        }
        if(c == '\t') //tab
        {
            cursorX += 4 * roundf(advance * fontData->scale);
        }
        if(c == ' ') //space
        {
            cursorX += roundf(advance * fontData->scale);
        }
        if(c >= '!' && c <= '~')
        {
            Rect glyphRect = {0};
            glyphRect.x = cursorX + fontBitMaps[c].xOffset;
            glyphRect.y = baseline + fontBitMaps[c].yOffset;
            glyphRect.width = fontBitMaps[c].width;
            glyphRect.height = fontBitMaps[c].height;
            
            RenderFontBitMap(buffer, fontBitMaps[c].bitMap, &glyphRect);
            
            cursorX += roundf(advance * fontData->scale);
            
            if(textBuffer[i + 1])
            {
                i32 kern = stbtt_GetCodepointKernAdvance(&fontData->fontInfo, textBuffer[i], textBuffer[i + 1]);
                cursorX += roundf(kern * fontData->scale);
            }
        }
    }
    
    for(i32 i = postStartIndex; i <= endIndex; i++)
    {
        char c = textBuffer[i];
        
        i32 advance = 0, lsb = 0;
        stbtt_GetCodepointHMetrics(&fontData->fontInfo, c, &advance, &lsb);
        
        if(c == '\n') //new line
        {
            baseline += fontData->lineGap + fontData->ascent - fontData->descent;
            cursorX = xPos;
        }
        if(c == '\t') //tab
        {
            cursorX += 4 * roundf(advance * fontData->scale);
        }
        if(c == ' ') //space
        {
            cursorX += roundf(advance * fontData->scale);
        }
        if(c >= '!' && c <= '~')
        {
            Rect glyphRect = {0};
            glyphRect.x = cursorX + fontBitMaps[c].xOffset;
            glyphRect.y = baseline + fontBitMaps[c].yOffset;
            glyphRect.width = fontBitMaps[c].width;
            glyphRect.height = fontBitMaps[c].height;
            
            RenderFontBitMap(buffer, fontBitMaps[c].bitMap, &glyphRect);
            
            cursorX += roundf(advance * fontData->scale);
            
            if(textBuffer[i + 1])
            {
                i32 kern = stbtt_GetCodepointKernAdvance(&fontData->fontInfo, textBuffer[i], textBuffer[i + 1]);
                cursorX += roundf(kern * fontData->scale);
            }
        }
    }
}
