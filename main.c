#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL2/SDL.h>

#include "types.h"
#include "font.c"
#include "text.c"
#include "render.c"

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

global SDL_Window *window;
global SDL_Renderer *renderer;
global SDL_Event event;

global Buffer buffer;

//NOTE(abhicv) : from ('!') = 33 to ('~') = 126
#define MAX_ASCII_CHAR 94 
global FontBitMap fontBitMaps[MAX_ASCII_CHAR];

global TextSequence textSeq;
u8 *textOriginalBuffer = NULL;

int main(int argc, char *argv[])
{
    if(SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        SDL_Log("Failed to initialize SDL : %s\n", SDL_GetError());
        return 1;
    }
    
    window = SDL_CreateWindow("SmallEd",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              SCREEN_WIDTH,
                              SCREEN_HEIGHT,
                              SDL_WINDOW_ALLOW_HIGHDPI);
    
    if(window == NULL)
    {
        SDL_Log("Failed to create SDL window : %s\n", SDL_GetError());
        return 1;
    }
    
    renderer = SDL_CreateRenderer(window, 3, SDL_RENDERER_ACCELERATED);
    
    if(renderer == NULL)
    {
        SDL_Log("Failed to create SDL renderer : %s\n", SDL_GetError());
        return 1;
    }
    
    //NOTE(abhicv): allocating memory for render buffer
    {
        buffer.data = malloc(sizeof(u32) * SCREEN_WIDTH * SCREEN_HEIGHT);
        buffer.width = SCREEN_WIDTH;
        buffer.height = SCREEN_HEIGHT;
    }
    
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR32, SDL_TEXTUREACCESS_TARGET, SCREEN_WIDTH, SCREEN_HEIGHT);
    
    FontData fontData = LoadFont("JetBrainsMono-Regular.ttf", 24);
    
    //NOTE(abhicv): font bitmaps loading
    {
        for(u8 n = 33; n < 127; n++)
        {
            //Bitmap Rasterizing
            fontBitMaps[n - 33].bitMap = stbtt_GetCodepointBitmap(&fontData.fontInfo, 0, fontData.scale, n, &fontBitMaps[n - 33].width, &fontBitMaps[n - 33].height, &fontBitMaps[n - 33].xOffset, &fontBitMaps[n - 33].yOffset);
#if 0
            int x0, x1, y0, y1;
            stbtt_GetCodepointBitmapBox(&fontData.fontInfo, n, 0, fontData.scale, &x0, &y0, &x1, &y1);
            fontBitMaps[n - 33].xOffset = x0;
            fontBitMaps[n - 33].yOffset = y0;
#endif
        }
    }
    
    Rect caret = {0, 0, 4, fontData.lineGap + fontData.ascent - fontData.descent};
    Rect lineHighlight = {0, 0, SCREEN_WIDTH, caret.height};
    
    Rect lineMargin = {0, 0, 0, SCREEN_HEIGHT};
    i32 a, l;
    stbtt_GetCodepointHMetrics(&fontData.fontInfo, '0', &a, &l);
    lineMargin.width = 3 * roundf(a * fontData.scale) + 5;
    
    textSeq.buffer = malloc(TEXT_BUFFER_SIZE);
    memset(&textSeq.buffer[0], 0, TEXT_BUFFER_SIZE);
    textSeq.gapSize = TEXT_BUFFER_SIZE;
    textSeq.preEndIndex = -1;
    textSeq.postStartIndex = TEXT_BUFFER_SIZE;
    
    b32 quit = false;
    SDL_StartTextInput();
    while(!quit)
    {
        while(SDL_PollEvent(&event) != 0)
        {
            switch(event.type)
            {
                case SDL_QUIT:
                quit = true;
                break;
                
                case SDL_KEYDOWN:
                if(event.key.keysym.sym == SDLK_BACKSPACE)
                {
                    DeleteItem(&textSeq);
                }
                else if(event.key.keysym.sym == SDLK_ESCAPE)
                {
                    quit = true;
                }
                else if(event.key.keysym.sym == SDLK_RETURN)
                {
                    InsertItem(&textSeq, '\n');
                }
                else if(event.key.keysym.sym == SDLK_TAB)
                {
                    InsertItem(&textSeq, '\t');
                }
                else if(event.key.keysym.sym == SDLK_LEFT)
                {
                    MoveLeft(&textSeq);
                }
                else if(event.key.keysym.sym == SDLK_RIGHT)
                {
                    MoveRight(&textSeq);
                }
                else if(event.key.keysym.sym == SDLK_UP)
                {
                    MoveUp(&textSeq);
                }
                else if(event.key.keysym.sym == SDLK_DOWN)
                {
                    MoveDown(&textSeq);
                }
                break;
                
                case SDL_TEXTINPUT:
                InsertItem(&textSeq, event.text.text[0]);
                //printf("%c\n", event.text.text[0]);
                break;
            }
        }
        
        ClearBuffer(&buffer, (Color){10, 10, 10, 0});
        
        //NOTE(abhicv): calculating caret position
        {
            caret.x = lineMargin.width;
            caret.y = 0;
            
            for(u32 n = 0; n < textSeq.preEndIndex + 1; n++)
            {
                if(textSeq.buffer[n] == '\n')
                {
                    caret.y += caret.height;
                    caret.x = lineMargin.width;
                }
                else if(textSeq.buffer[n] == '\t')
                {
                    i32 advance, lsb;
                    stbtt_GetCodepointHMetrics(&fontData.fontInfo, '\t', &advance, &lsb);
                    caret.x += 4 * roundf(advance * fontData.scale);
                }
                else
                {
                    i32 advance, lsb;
                    stbtt_GetCodepointHMetrics(&fontData.fontInfo, textSeq.buffer[n], &advance, &lsb);
                    caret.x += roundf(advance * fontData.scale);
                    //caret.width = roundf(advance * fontData.scale);
                }
            }
            
            lineHighlight.y = caret.y;
        }
        
        //RENDERING
        {
            DrawRect(&buffer, &lineHighlight, (Color){60, 60, 60, 255});
            DrawRect(&buffer, &lineMargin, (Color){20, 20, 20, 255});
            
            //NOTE(abhicv): line numbers
            {
                u32 p = lineMargin.y;
                for(u32 n = 0; n <= 27; n++)
                {
                    char number[3] = {0};
                    sprintf(&number[0], "%d\0", n);
                    RenderTextBuffer(&buffer, &number[0], &fontData, fontBitMaps, lineMargin.x + 3, p, 0);
                    p += fontData.lineGap + fontData.ascent - fontData.descent;
                }
            }
            
            //NOTE(abhicv): creating new buffer combining pre and post span
            {
                u32 size = textSeq.preEndIndex + 1 + TEXT_BUFFER_SIZE - textSeq.postStartIndex + 1; 
                textOriginalBuffer = (u8*)malloc(size);
                
                memset(&textOriginalBuffer[0], 0, size);
                memcpy(&textOriginalBuffer[0], &textSeq.buffer[0], textSeq.preEndIndex + 1);
                memcpy(&textOriginalBuffer[textSeq.preEndIndex + 1], &textSeq.buffer[textSeq.postStartIndex], TEXT_BUFFER_SIZE - textSeq.postStartIndex);
                
                RenderTextBuffer(&buffer, &textOriginalBuffer[0], &fontData, fontBitMaps, lineMargin.width, 0, 0);
                free(textOriginalBuffer);
            }
            
            DrawRect(&buffer, &caret, (Color){255, 255, 0, 255});
            
            SDL_UpdateTexture(texture, NULL, buffer.data, 4 * buffer.width);
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);
        }
    }
    
    SDL_DestroyTexture(texture);
    free(buffer.data);
    buffer.data = NULL;
    
    return 0;
}