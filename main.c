#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL2/SDL.h>

#include "types.h"
#include "font.c"
#include "render.c"

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

global SDL_Window *window;
global SDL_Renderer *renderer;
global SDL_Event event;

global Buffer buffer;

#define MAX_ASCII_CHAR 94 //from ('!') = 33 to ('~') = 126
global FontBitMap fontBitMaps[MAX_ASCII_CHAR];

#define TEXT_BUFFER_SIZE 500
u8 *textOriginalBuffer = NULL;

typedef struct TextSequence
{
    u8 *buffer;
    i32 preEndIndex;
    i32 postStartIndex;
    u32 gapSize;
    
} TextSequence;

global TextSequence textSeq;

void InsertItem(TextSequence *tSeq, u8 item)
{
    if(tSeq->gapSize > 0)
    {
        tSeq->buffer[++tSeq->preEndIndex] = item;
        tSeq->gapSize--;
        //printf("pre : %d, post : %d, gap : %d\n", tSeq->preEndIndex, tSeq->postStartIndex, tSeq->gapSize);
    }
    else
    {
        //resize text buffer to create new gap
    }
}

void DeleteItem(TextSequence *tSeq)
{
    if(tSeq->preEndIndex > -1)
    {
        tSeq->preEndIndex--;
        tSeq->gapSize++;
        //printf("pre : %d, post : %d, gap : %d\n", tSeq->preEndIndex, tSeq->postStartIndex, tSeq->gapSize);
    }
}

void MoveLeft(TextSequence *tSeq)
{
    if(tSeq->preEndIndex > -1)
    {
        tSeq->buffer[--tSeq->postStartIndex] = tSeq->buffer[tSeq->preEndIndex--];
        //printf("pre : %d, post : %d, gap : %d\n", tSeq->preEndIndex, tSeq->postStartIndex, tSeq->gapSize);
    }
}

void MoveRight(TextSequence *tSeq)
{
    if(tSeq->postStartIndex < TEXT_BUFFER_SIZE)
    {
        tSeq->buffer[++tSeq->preEndIndex] = tSeq->buffer[tSeq->postStartIndex++];
        //printf("pre : %d, post : %d, gap : %d\n", tSeq->preEndIndex, tSeq->postStartIndex, tSeq->gapSize);
    }
}

void MoveUp(TextSequence *tSeq)
{
    u32 currentLineCharCount = 0;
    
    while(tSeq->buffer[tSeq->preEndIndex] != '\n' && tSeq->preEndIndex > -1)
    {
        MoveLeft(tSeq);
        currentLineCharCount++;
    }
    //printf("c : %d\n", currentLineCharCount);
    MoveLeft(tSeq);
    
    u32 upperLineCharCount = 0;
    while(tSeq->buffer[tSeq->preEndIndex] != '\n' && tSeq->preEndIndex > -1)
    {
        MoveLeft(tSeq);
        upperLineCharCount++;
    }
    //printf("c : %d\n", upperLineCharCount);
    
    if(upperLineCharCount >= currentLineCharCount)
    {
        for(u32 n = 0; n < currentLineCharCount; n++)
        {
            MoveRight(tSeq);
        }
    }
    else
    {
        for(u32 n = 0; n < upperLineCharCount; n++)
        {
            MoveRight(tSeq);
        }
    }
}

void MoveDown(TextSequence *tSeq)
{
    u32 currentLineCharCount = 0;
    
    while(tSeq->buffer[tSeq->preEndIndex] != '\n' && tSeq->preEndIndex > -1)
    {
        MoveLeft(tSeq);
        currentLineCharCount++;
    }
    //printf("c : %d\n", currentLineCharCount);
    
    while(tSeq->buffer[tSeq->postStartIndex] != '\n' && tSeq->postStartIndex < TEXT_BUFFER_SIZE)
    {
        MoveRight(tSeq);
    }
    
    MoveRight(tSeq);
    
    u32 n = 0;
    while(n < currentLineCharCount && tSeq->buffer[tSeq->postStartIndex] != '\n' && tSeq->postStartIndex < TEXT_BUFFER_SIZE)
    {
        MoveRight(tSeq);
        n++;
    }
}

int main(int argc, char *argv[])
{
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
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
    
    if (window == NULL)
    {
        SDL_Log("Failed to create SDL window : %s\n", SDL_GetError());
        return 1;
    }
    
    renderer = SDL_CreateRenderer(window, 3, SDL_RENDERER_ACCELERATED);
    
    if (renderer == NULL)
    {
        SDL_Log("Failed to create SDL renderer : %s\n", SDL_GetError());
        return 1;
    }
    
    //allocating memory for render buffer
    {
        buffer.data = malloc(sizeof(u32) * SCREEN_WIDTH * SCREEN_HEIGHT);
        buffer.width = SCREEN_WIDTH;
        buffer.height = SCREEN_HEIGHT;
    }
    
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, SCREEN_WIDTH, SCREEN_HEIGHT);
    
    FontData fontData = LoadFont("C:/Windows/Fonts/consola.ttf", 22);
    
    //font bitmaps loading
    {
        for(u8 n = 33; n < 127; n++)
        {
            //Bitmap Rasterizing
            fontBitMaps[n - 33].bitMap = stbtt_GetCodepointBitmap(&fontData.fontInfo, 0, fontData.scale, n, &fontBitMaps[n - 33].width, &fontBitMaps[n - 33].height, &fontBitMaps[n - 33].xOffset, &fontBitMaps[n - 33].yOffset);
        }
    }
    
    Rect caret = {0, 0, 10, fontData.lineGap + fontData.ascent - fontData.descent};
    Rect lineHighlight = {0, 0, SCREEN_WIDTH, caret.height};
    
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
        
        ClearBuffer(&buffer, (Color){0, 0, 0, 0});
        
        //calculating caret position
        {
            caret.x = 0;
            caret.y = 0;
            
            for(u32 n = 0; n < textSeq.preEndIndex + 1; n++)
            {
                if(textSeq.buffer[n] == '\n')
                {
                    caret.y += caret.height;
                    caret.x = 0;
                }
                else if(textSeq.buffer[n] == '\t')
                {
                    i32 advance, lsb;
                    stbtt_GetCodepointHMetrics(&fontData.fontInfo, ' ', &advance, &lsb);
                    caret.x += 4 * roundf(advance * fontData.scale);
                }
                else
                {
                    i32 advance, lsb;
                    stbtt_GetCodepointHMetrics(&fontData.fontInfo, textSeq.buffer[n], &advance, &lsb);
                    caret.x += roundf(advance * fontData.scale);
                    caret.width = roundf(advance * fontData.scale);
                }
            }
        }
        lineHighlight.y = caret.y;
        
        DrawRect(&buffer, &lineHighlight, (Color){50, 50, 50, 0});
        DrawRect(&buffer, &caret, (Color){255, 255, 0, 0});
        
        //creating new buffer combining pre and post span
        
        //NOTE(abhicv) : not sure why its not working
        //textOriginalBuffer = (u8*)malloc(textSeq.preEndIndex + 1 + TEXT_BUFFER_SIZE - textSeq.postStartIndex);
        //memset(&textOriginalBuffer[0], 0, textSeq.preEndIndex + 1 + TEXT_BUFFER_SIZE - textSeq.postStartIndex);
        
        textOriginalBuffer = (u8*)malloc(TEXT_BUFFER_SIZE);
        memset(&textOriginalBuffer[0], 0, TEXT_BUFFER_SIZE);
        
        memcpy(&textOriginalBuffer[0], &textSeq.buffer[0], textSeq.preEndIndex + 1);
        memcpy(&textOriginalBuffer[textSeq.preEndIndex + 1], &textSeq.buffer[textSeq.postStartIndex], TEXT_BUFFER_SIZE - textSeq.postStartIndex);
        
        RenderTextBuffer(&buffer, &textOriginalBuffer[0], &fontData, fontBitMaps, 0, 0, 0);
        free(textOriginalBuffer);
        
        SDL_UpdateTexture(texture, NULL, buffer.data, 4 * buffer.width);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }
    
    SDL_DestroyTexture(texture);
    free(buffer.data);
    buffer.data = NULL;
    
    return 0;
}