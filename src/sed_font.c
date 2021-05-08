#include "sed_font.h"

FontData* LoadFont(const u8 *fontFileName, f32 fontSize)
{
    FontData *fontData = 0;
    fontData = (FontData*)malloc(sizeof(FontData));
    
    FILE *fontFileHandle = fopen(fontFileName, "rb");
    if(fontFileHandle != NULL)
    {
        fseek(fontFileHandle, 0, SEEK_END);
        u32 size = ftell(fontFileHandle); 
        fseek(fontFileHandle, 0, SEEK_SET);
        
        fontData->rawFontData = (u8*)AllocatePersistentMemory(size);
        
        i32 readStatus = fread(fontData->rawFontData, 1, size, fontFileHandle);
        
        if(readStatus == size)
        {
            printf("Read all bytes successfully from '%s'\n", fontFileName);
            stbtt_InitFont(&fontData->fontInfo, fontData->rawFontData, stbtt_GetFontOffsetForIndex(fontData->rawFontData, 0));
        }
        
        fclose(fontFileHandle);
    }
    else
    {
        printf("Font file '%s' opening failed!!\n", fontFileName);
        return NULL;
    }
    
    float s = stbtt_ScaleForMappingEmToPixels(&fontData->fontInfo, 1) / stbtt_ScaleForPixelHeight(&fontData->fontInfo, 1);
    
    //TODO(abhicv): resize atlas bitmap if defualt dimensions not suitable
    fontData->atlasBitMap.width = 512;
    fontData->atlasBitMap.height = 512;
    fontData->atlasBitMap.pixels = (u8*)AllocatePersistentMemory(fontData->atlasBitMap.width * fontData->atlasBitMap.height);
    
    stbtt_BakeFontBitmap(fontData->rawFontData, 0, fontSize * s, fontData->atlasBitMap.pixels,
                         fontData->atlasBitMap.width, fontData->atlasBitMap.height, 0, 256, fontData->charDatas);
    
    f32 scale = stbtt_ScaleForMappingEmToPixels(&fontData->fontInfo, fontSize);
    
    i32 ascent = 0;
    i32 descent = 0; 
    i32 lineGap = 0;
    
    stbtt_GetFontVMetrics(&fontData->fontInfo, &ascent, &descent, &lineGap);
    
    fontData->fontSize = fontSize;
    fontData->lineHeight = (ascent - descent + lineGap) * scale + 0.5f;
    
    for(u32 n = 0; n < 256; n++)
    {
        fontData->charDatas[n].yoff += (ascent * scale) + 0.5f;
        fontData->charDatas[n].xadvance = floor(fontData->charDatas[n].xadvance);
    }
    
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

Color GetBufferPixelColor(Buffer *buffer, u32 x, u32 y)
{
    Color color = {0};
    color.r = (buffer->data[x + y * buffer->width] & 0x000000FF) >> 0;
    color.g = (buffer->data[x + y * buffer->width] & 0x0000FF00) >> 8;
    color.b = (buffer->data[x + y * buffer->width] & 0x00FF0000) >> 16;
    color.a = (buffer->data[x + y * buffer->width] & 0xFF000000) >> 24;
    return color;
}

void RenderFontBitMap(Buffer *renderBuffer, Rect *destRect, BitMap *atlasBitMap, Rect *srcRect, Color color, Rect clipRect)
{
    if(renderBuffer != NULL && atlasBitMap != NULL)
    {
        u32 cY = srcRect->y;
        
        for(u32 y = destRect->y; y < (destRect->y + destRect->height); y++)
        {
            u32 cX = srcRect->x;
            for(u32 x = destRect->x; x < (destRect->x + destRect->width); x++)
            {
                if(x >= clipRect.x && y >= clipRect.y && x < (clipRect.x + clipRect.width) && y < (clipRect.y + clipRect.height))
                {
                    u8 alpha = atlasBitMap->pixels[cX + cY * atlasBitMap->width];
                    
                    Color dst = GetBufferPixelColor(renderBuffer, x, y);
                    Color src = {255, 255, 255, alpha};
                    Color blendColor = BlendPixel(dst, src, color);
                    
                    renderBuffer->data[x + y * renderBuffer->width] = 
                        PACK_RGBA_INTO_U32(blendColor);
                }
                cX++;
            }
            cY++;
        }
    }
}

//renders a text onto the screenand return last x draw position 
u32 RenderText(Buffer *renderBuffer, u8 *textBuffer, u32 size, FontData *fontData, u32 xPos, u32 yPos, Color color, Rect clipRect)
{
    u32 x = xPos;
    u32 y = yPos;
    
    for(u32 i = 0; i < size; i++)
    {
        char c = textBuffer[i];
        
        if(c == '\t')
        {
            x += 4 * fontData->charDatas[(u32)(' ')].xadvance;
        }
        else if(RENDERABLE_CHAR(c))
        {
            Rect srcRect = {0};
            srcRect.x = fontData->charDatas[(u32)c].x0;
            srcRect.y = fontData->charDatas[(u32)c].y0;
            srcRect.width = fontData->charDatas[(u32)c].x1 - fontData->charDatas[(u32)c].x0;
            srcRect.height = fontData->charDatas[(u32)c].y1 - fontData->charDatas[(u32)c].y0;
            
            Rect destRect = {0};
            destRect.x = x + fontData->charDatas[(u32)c].xoff;
            destRect.y = y + fontData->charDatas[(u32)c].yoff;
            destRect.width = srcRect.width;
            destRect.height = srcRect.height;
            
            RenderFontBitMap(renderBuffer, &destRect, &fontData->atlasBitMap, &srcRect, color, clipRect);
            
            x += fontData->charDatas[(u32)c].xadvance;
        }
        else
        {
            x += fontData->charDatas[(u32)(' ')].xadvance;
        }
    }
    
    return x;
}

void RenderTextSequence(Buffer *renderBuffer, TextSequence *tSeq, 
                        FontData *fontData, 
                        u32 xPos, u32 yPos, b32 activeTSeq, Rect clipRect)
{
    u32 x = xPos;
    u32 y = yPos;
    
    u32 colorIndex = 0;
    
    //from 0 index to preEndIndex
    for(u32 i = 0; i < tSeq->preSize; i++)
    {
        u8 c = tSeq->buffer[i];
        
        if(c == '\t')
        {
            x += 4 * fontData->charDatas[(u32)(' ')].xadvance;
        }
        else if(RENDERABLE_CHAR(c))
        {
            Rect srcRect = {0};
            srcRect.x = fontData->charDatas[(u32)c].x0;
            srcRect.y = fontData->charDatas[(u32)c].y0;
            srcRect.width = fontData->charDatas[(u32)c].x1 - fontData->charDatas[(u32)c].x0;
            srcRect.height = fontData->charDatas[(u32)c].y1 - fontData->charDatas[(u32)c].y0;
            
            Rect destRect = {0};
            destRect.x = x + fontData->charDatas[(u32)c].xoff;
            destRect.y = y + fontData->charDatas[(u32)c].yoff;
            destRect.width = srcRect.width;
            destRect.height = srcRect.height;
            
            Color color = ColorLookUpTable[tSeq->colorIndexBuffer[colorIndex]];
            
            RenderFontBitMap(renderBuffer, &destRect, &fontData->atlasBitMap, &srcRect, color, clipRect);
            
            x += fontData->charDatas[(u32)c].xadvance;
            
            if(i < (tSeq->preSize - 2))
            {
                f32 scale = stbtt_ScaleForMappingEmToPixels(&fontData->fontInfo, fontData->fontSize);
                x += scale * stbtt_GetCodepointKernAdvance(&fontData->fontInfo, c, tSeq->buffer[i + 1]);
            }
            
        }
        else
        {
            x += fontData->charDatas[(u32)(' ')].xadvance;
        }
        
        colorIndex++;
    }
    
    //from postStartIndex to buffer end
    if(tSeq->postSize > 0)
    {
        u32 postStartIndex = tSeq->bufferCapacity - tSeq->postSize;
        
        for(u32 i = postStartIndex; i < tSeq->bufferCapacity; i++)
        {
            u8 c = tSeq->buffer[i];
            
            if(c == '\t')
            {
                x += 4 * fontData->charDatas[(u32)(' ')].xadvance;
            }
            else if(RENDERABLE_CHAR(c))
            {
                Rect srcRect = {0};
                srcRect.x = fontData->charDatas[(u32)c].x0;
                srcRect.y = fontData->charDatas[(u32)c].y0;
                srcRect.width = fontData->charDatas[(u32)c].x1 - fontData->charDatas[(u32)c].x0;
                srcRect.height = fontData->charDatas[(u32)c].y1 - fontData->charDatas[(u32)c].y0;
                
                Rect destRect = {0};
                destRect.x = x + fontData->charDatas[(u32)c].xoff;
                destRect.y = y + fontData->charDatas[(u32)c].yoff;
                destRect.width = srcRect.width;
                destRect.height = srcRect.height;
                
                Color color = ColorLookUpTable[tSeq->colorIndexBuffer[colorIndex]];
                
                if(activeTSeq && i == postStartIndex)
                {
                    color = (Color){0, 0, 0, 255};
                }
                
                RenderFontBitMap(renderBuffer, &destRect, &fontData->atlasBitMap, &srcRect, color, clipRect);
                
                x += fontData->charDatas[(u32)c].xadvance;
                
                if(i < (tSeq->bufferCapacity - 2))
                {
                    f32 scale = stbtt_ScaleForMappingEmToPixels(&fontData->fontInfo, fontData->fontSize);
                    x += scale * stbtt_GetCodepointKernAdvance(&fontData->fontInfo, c, tSeq->buffer[i + 1]);
                }
            }
            else
            {
                x += fontData->charDatas[(u32)(' ')].xadvance;
            }
            colorIndex++;
        }
    }
}

void RenderTextSequenceSimple(Buffer *renderBuffer, TextSequence *tSeq, 
                              FontData *fontData, 
                              u32 xPos, u32 yPos, Color color, Rect clipRect)
{
    u32 x = xPos;
    u32 y = yPos;
    
    //from 0 index to preEndIndex
    for(u32 i = 0; i < tSeq->preSize; i++)
    {
        u8 c = tSeq->buffer[i];
        
        if(c == '\t')
        {
            x += 4 * fontData->charDatas[(u32)(' ')].xadvance;
        }
        else if(RENDERABLE_CHAR(c))
        {
            Rect srcRect = {0};
            srcRect.x = fontData->charDatas[(u32)c].x0;
            srcRect.y = fontData->charDatas[(u32)c].y0;
            srcRect.width = fontData->charDatas[(u32)c].x1 - fontData->charDatas[(u32)c].x0;
            srcRect.height = fontData->charDatas[(u32)c].y1 - fontData->charDatas[(u32)c].y0;
            
            Rect destRect = {0};
            destRect.x = x + fontData->charDatas[(u32)c].xoff;
            destRect.y = y + fontData->charDatas[(u32)c].yoff;
            destRect.width = srcRect.width;
            destRect.height = srcRect.height;
            
            RenderFontBitMap(renderBuffer, &destRect, &fontData->atlasBitMap, &srcRect, color, clipRect);
            
            x += fontData->charDatas[(u32)c].xadvance;
        }
        else
        {
            x += fontData->charDatas[(u32)(' ')].xadvance;
        }
    }
    
    //from postStartIndex to buffer end
    if(tSeq->postSize > 0)
    {
        u32 postStartIndex = (tSeq->bufferCapacity - tSeq->postSize);
        
        for(u32 i = postStartIndex; i < tSeq->bufferCapacity; i++)
        {
            u8 c = tSeq->buffer[i];
            
            if(c == '\t')
            {
                x += 4 * fontData->charDatas[(u32)(' ')].xadvance;
            }
            else if(RENDERABLE_CHAR(c))
            {
                Rect srcRect = {0};
                srcRect.x = fontData->charDatas[(u32)c].x0;
                srcRect.y = fontData->charDatas[(u32)c].y0;
                srcRect.width = fontData->charDatas[(u32)c].x1 - fontData->charDatas[(u32)c].x0;
                srcRect.height = fontData->charDatas[(u32)c].y1 - fontData->charDatas[(u32)c].y0;
                
                Rect destRect = {0};
                destRect.x = x + fontData->charDatas[(u32)c].xoff;
                destRect.y = y + fontData->charDatas[(u32)c].yoff;
                destRect.width = srcRect.width;
                destRect.height = srcRect.height;
                
                RenderFontBitMap(renderBuffer, &destRect, &fontData->atlasBitMap, &srcRect, color, clipRect);
                
                x += fontData->charDatas[(u32)c].xadvance;
            }
            else
            {
                x += fontData->charDatas[(u32)(' ')].xadvance;
            }
        }
    }
}
