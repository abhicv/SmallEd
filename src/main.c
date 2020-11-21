#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL2/SDL.h>

#include "types.h"
#include "font.c"
#include "text.c"
#include "file.c"
#include "render.c"

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

global SDL_Window *window;
global SDL_Renderer *renderer;
global SDL_Event event;

//NOTE(abhicv) : from ('!') = 33 to ('~') = 126
#define MAX_ASCII_CHAR 94

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
                              0);
    
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
    
    Buffer buffer = {0};
    buffer.data = malloc(sizeof(u32) * SCREEN_WIDTH * SCREEN_HEIGHT);
    buffer.width = SCREEN_WIDTH;
    buffer.height = SCREEN_HEIGHT;
    
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR32, SDL_TEXTUREACCESS_TARGET, SCREEN_WIDTH, SCREEN_HEIGHT);
    if(texture == NULL)
    {
        printf("Failed to create SDL texture : %s\n", SDL_GetError());
    }
    
    FontData fontData = LoadFont("font/Inconsolata.ttf", 25);
    
    FontBitMap fontBitMaps[MAX_ASCII_CHAR];
    
    //NOTE(abhicv): ascii character bitmaps loading
    {
        for(u8 n = 33; n < 127; n++)
        {
            //Bitmap Rasterizing
            fontBitMaps[n - 33].bitMap = stbtt_GetCodepointBitmap(&fontData.fontInfo, 0, fontData.scale, n, &fontBitMaps[n - 33].width, &fontBitMaps[n - 33].height, &fontBitMaps[n - 33].xOffset, &fontBitMaps[n - 33].yOffset);
        }
    }
    
    Rect caret = {0};
    caret.x = 0;
    caret.y = 0;
    caret.width = 12;
    caret.height = fontData.lineGap + fontData.ascent - fontData.descent;
    
    Rect lineHighlight = {0};
    lineHighlight.x = 0;
    lineHighlight.y = 0;
    lineHighlight.width = SCREEN_WIDTH;
    lineHighlight.height = caret.height;
    
    i32 a = 0, l = 0;
    stbtt_GetCodepointHMetrics(&fontData.fontInfo, '0', &a, &l);
    
    Rect lineMargin = {0};
    lineMargin.x = 0;
    lineMargin.y = 0;
    lineMargin.width = 3 * roundf(a * fontData.scale) + 5;
    lineMargin.height = SCREEN_HEIGHT;
    
    TextSequence textSeq = {0};
    textSeq.buffer = malloc(TEXT_BUFFER_SIZE);
    memset(&textSeq.buffer[0], -1, TEXT_BUFFER_SIZE);
    
    //reading a file
    {
        FILE *file = fopen("test_file.c", "r");
        fseek(file, 0, SEEK_END);
        u32 size = ftell(file);
        fseek(file, 0, SEEK_SET);
        fread(&textSeq.buffer[TEXT_BUFFER_SIZE - size], 1, size, file);
        fclose(file);
        
        textSeq.gapSize = TEXT_BUFFER_SIZE;
        textSeq.preEndIndex = -1;
        textSeq.postStartIndex = TEXT_BUFFER_SIZE - size;
        textSeq.bufferSize = TEXT_BUFFER_SIZE;
    }
    
    u32 currentActiveLineNumber = 1;
    u32 numOfLines = 0;
    
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
                break;
            }
        }
        
        ClearBuffer(&buffer, (Color){0, 0, 0, 0});
        
        //NOTE(abhicv): calculating caret position, active line number, total no. of lines
        {
            caret.x = lineMargin.width;
            caret.y = 0;
            currentActiveLineNumber = 1;
            numOfLines = 0;
            
            for(u32 n = 0; n < textSeq.preEndIndex + 1; n++)
            {
                if(textSeq.buffer[n] == '\n')
                {
                    caret.y += caret.height;
                    caret.x = lineMargin.width;
                    
                    currentActiveLineNumber++;
                    numOfLines++;
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
                }
            }
            lineHighlight.y = caret.y;
            
            for(u32 n = textSeq.postStartIndex; n < TEXT_BUFFER_SIZE; n++)
            {
                if(textSeq.buffer[n] == '\n')
                {
                    numOfLines++;
                }
            }
        }
        
        //Rendering
        {
            DrawRectWire(&buffer, &lineMargin, (Color){50, 50, 50, 255});
            
            DrawRect(&buffer, &lineHighlight, (Color){40, 40, 40, 255});
            DrawRect(&buffer, &caret, (Color){255, 255, 0, 255});
            
            //NOTE(abhicv): line numbers
            {
                u32 p = lineMargin.y;
                for(u32 n = 1; n < 30; n++)
                {
                    char number[3] = {0, 0, 0};
                    sprintf(&number[0], "%d\0", n);
                    RenderTextBuffer(&buffer, &number[0], &fontData, fontBitMaps, lineMargin.x + 4, p, 0, 2);
                    p += fontData.lineGap + fontData.ascent - fontData.descent;
                }
            }
            
            //NOTE(abhicv): creating new buffer combining pre and post span buffers
            {
                u32 size = (textSeq.preEndIndex + 1) + (TEXT_BUFFER_SIZE - textSeq.postStartIndex) + 1;
                u8 *textOriginalBuffer = (u8*)malloc(size);
                
                if(textOriginalBuffer != NULL)
                {
                    memset(&textOriginalBuffer[0], 0, size);
                    memcpy(&textOriginalBuffer[0], &textSeq.buffer[0], textSeq.preEndIndex + 1);
                    memcpy(&textOriginalBuffer[textSeq.preEndIndex + 1], &textSeq.buffer[textSeq.postStartIndex], TEXT_BUFFER_SIZE - textSeq.postStartIndex);
                    
                    RenderTextBuffer(&buffer, &textOriginalBuffer[0], &fontData, fontBitMaps, lineMargin.width, 0, 0, size - 1);
                    free(textOriginalBuffer);
                }
            }
            
            SDL_UpdateTexture(texture, NULL, buffer.data, 4 * buffer.width);
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);
        }
    }
    
    SDL_DestroyTexture(texture);
    free(buffer.data);
    buffer.data = NULL;
    SDL_Quit();
    
    return 0;
}
