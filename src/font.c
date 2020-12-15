#include "font.h"
#include "debug.h"
#include "lexer.h"

FontData LoadFont(const char *fontFile, u32 fontSize)
{
    FontData fontData = {0};
    
    FILE *fontFileHandle = fopen(fontFile, "rb");
    if(fontFileHandle != NULL)
    {
        fseek(fontFileHandle, 0, SEEK_END);
        u32 size = ftell(fontFileHandle); 
        fseek(fontFileHandle, 0, SEEK_SET);
        
        fontData.rawData = (u8*)malloc(size);
        i32 readStatus = fread(fontData.rawData, 1, size, fontFileHandle);
        
        if(readStatus == size)
        {
            printf("Read all bytes successfully from '%s'\n", fontFile);
            stbtt_InitFont(&fontData.fontInfo, fontData.rawData, stbtt_GetFontOffsetForIndex(fontData.rawData, 0));
        }
        
        fclose(fontFileHandle);
    }
    
    i32 ascent = 0;
    i32 descent = 0; 
    i32 lineGap = 0;
    
    stbtt_GetFontVMetrics(&fontData.fontInfo, &ascent, &descent, &lineGap);
    f32 scale = stbtt_ScaleForPixelHeight(&fontData.fontInfo, fontSize);
    
    fontData.size = fontSize;
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

void RenderFontBitMap(Buffer *buffer, u8 *bitMap, Rect *destRect, Color color)
{
    if(buffer != NULL && bitMap != NULL)
    {
        for(u32 y = destRect->y; y < (destRect->y + destRect->height); y++)
        {
            for(u32 x = destRect->x; x < (destRect->x + destRect->width); x++)
            {
                if(x >= 0 && y >= 0 && x < buffer->width && y < buffer->height)
                {
                    u32 bX = x - destRect->x;
                    u32 bY = y - destRect->y;
                    u8 alpha = bitMap[bX + bY * destRect->width];
                    
                    Color dst = GetBufferPixelColor(buffer, x, y);
                    //Color src = {alpha, alpha, alpha, alpha};
                    Color src = {255, 255, 255, alpha};
                    Color blendColor = BlendPixel(dst, src, color);
                    
                    buffer->data[x + y * buffer->width] = 
                        (blendColor.r << 24) | 
                        (blendColor.g << 16) | 
                        (blendColor.b << 8) | 
                        (blendColor.a  << 0);
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
            
            Color color = {255, 255, 255 ,255};
            
            RenderFontBitMap(buffer, fontBitMaps[c].bitMap, &glyphRect, color);
            
            cursorX += roundf(advance * fontData->scale);
            
            if(textBuffer[i + 1])
            {
                i32 kern = stbtt_GetCodepointKernAdvance(&fontData->fontInfo, textBuffer[i], textBuffer[i + 1]);
                cursorX += roundf(kern * fontData->scale);
            }
        }
    }
}

void RenderTextBuffer(Buffer *renderBuffer, u8 *textBuffer, FontData *fontData, FontBitMap *fontBitMaps, u8* colorIndexBuffer, u32 xPos, u32 yPos, 
                      i32 preEndIndex, i32 postStartIndex,
                      u32 startIndex, u32 endIndex)
{
    u32 cursorX = xPos;
    u32 baseline = yPos + fontData->ascent + fontData->lineGap;
    
    u32 colorIndex = startIndex;
    
    //from start index to preEndIndex
    for(i32 i = startIndex; i <= preEndIndex; i++)
    {
        u8 c = textBuffer[i];
        
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
            
            //Color color = {255, 255, 255, 255};
            Color color = ColorLookUpTable[colorIndexBuffer[colorIndex]];
            
            RenderFontBitMap(renderBuffer, fontBitMaps[c].bitMap, &glyphRect, color);
            
            cursorX += roundf(advance * fontData->scale);
            
            if(textBuffer[i + 1])
            {
                i32 kern = stbtt_GetCodepointKernAdvance(&fontData->fontInfo, textBuffer[i], textBuffer[i + 1]);
                cursorX += roundf(kern * fontData->scale);
            }
        }
        colorIndex++;
    }
    
    //from postStartIndex to endIndex
    for(i32 i = postStartIndex; i <= endIndex; i++)
    {
        u8 c = textBuffer[i];
        
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
            
            //Color color = {255, 255, 255, 255};
            
            Color color = ColorLookUpTable[colorIndexBuffer[colorIndex]];
            if(i == postStartIndex)
            {
                color.r = color.g = color.b = 0;
            }
            
            RenderFontBitMap(renderBuffer, fontBitMaps[c].bitMap, &glyphRect, color);
            
            cursorX += roundf(advance * fontData->scale);
            
            if(textBuffer[i + 1])
            {
                i32 kern = stbtt_GetCodepointKernAdvance(&fontData->fontInfo, textBuffer[i], textBuffer[i + 1]);
                cursorX += roundf(kern * fontData->scale);
            }
        }
        colorIndex++;
    }
}
