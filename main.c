#include <stdio.h>
#include <SDL2/SDL.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include "types.h"
#include "render.c"

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

global SDL_Window *window;
global SDL_Renderer *renderer;
global SDL_Event event;

global Buffer buffer;

global char textBuffer[1024];
global u32 cIndex = 0;

char ttf_buffer[1 << 25];

int main(int argc, char *argv[])
{
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        SDL_Log("Failed to initialize SDL : %s\n", SDL_GetError());
        return 1;
    }
    
    window = SDL_CreateWindow("SmallEd",
							  SDL_WINDOWPOS_CENTERED,
							  SDL_WINDOWPOS_CENTERED,
							  SCREEN_WIDTH,
							  SCREEN_HEIGHT,
                              0);
    if (window == NULL)
    {
        SDL_Log("Failed to create SDL window : %s\n", SDL_GetError());
        //return 1;
    }
    
    //using software rendering
    renderer = SDL_CreateRenderer(window, 3, SDL_RENDERER_ACCELERATED);
    
    if (renderer == NULL)
    {
        SDL_Log("Failed to create SDL renderer : %s\n", SDL_GetError());
        //return 1;
    }
    
    //allocating memory for buffer
    buffer.data = malloc(sizeof(u32) * SCREEN_WIDTH * SCREEN_HEIGHT);
    buffer.width = SCREEN_WIDTH;
    buffer.height = SCREEN_HEIGHT;
    
    Rect rect = {0, 0, 30, SCREEN_HEIGHT};
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, SCREEN_WIDTH, SCREEN_HEIGHT);
    
    stbtt_fontinfo font;
    u32 w, h;
    u8 *bitmap = NULL;
    Rect charRect = {0, 0, 0, 0};
    FILE *fontFile = fopen("C:/Windows/Fonts/CourierPrime-Regular.ttf", "rb");
    if(fontFile != NULL)
    {
        fread(ttf_buffer, 1, 1 << 25, fontFile);
        stbtt_InitFont(&font, ttf_buffer, stbtt_GetFontOffsetForIndex(ttf_buffer,0));
    }
    fclose(fontFile);
    
    b32 quit = false;
    while(!quit)
    {
        while(SDL_PollEvent(&event) != 0)
        {
            switch(event.type)
            {
                case SDL_QUIT:
                quit = true;
                continue;
                break;
                
                case SDL_KEYDOWN:
                if(event.key.keysym.sym == SDLK_BACKSPACE)
                {
                    cIndex--;
                    textBuffer[cIndex] = 0;
                    //printf("%s\n", textBuffer);
                }
                else if(event.key.keysym.sym == SDLK_RETURN)
                {
                    textBuffer[cIndex] = '\n';
                    //printf("%s\n", textBuffer);
                    cIndex++;
                }
                break;
                
                case SDL_KEYUP:
                char c = event.key.keysym.sym;
                if(c >= ' ' && c <= '~')
                {
                    textBuffer[cIndex] = c;
                    //printf("%s\n", textBuffer);
                    cIndex++;
                }
                break;
            }
        }
        
        ClearBuffer(&buffer, (Color){20, 20, 20});
        
        //DrawCircle(&buffer, 300, 300, 50, (Color){255, 0, 0});
        //DrawRectRounded(&buffer, &rect, 20, (Color){255, 0, 0});
        {
            u32 xStart = charRect.x;
            u32 yStart = charRect.y;
            u32 size = 30;
            
            u32 i = 0;
            char c = textBuffer[i];
            while(c != 0)
            {
                if(c == '\n')
                {
                    charRect.y += size;
                    charRect.x = xStart - size;
                }
                if(c != ' ' && c != '\n')
                {
                    u32 xOffset, yOffset;
                    bitmap = stbtt_GetCodepointBitmap(&font, 0, stbtt_ScaleForPixelHeight(&font, size), textBuffer[i], &charRect.width, &charRect.height, &xOffset, &yOffset);
                    //charRect.y += yOffset;
                    //charRect.x += xOffset;
                    //DrawRectWire(&buffer, &charRect, (Color){255, 0, 0, 0});
                    RenderFontBitMap(&buffer, bitmap, &charRect);
                    free(bitmap);
                }
                charRect.x += size / 2;
                i++;
                c = textBuffer[i];
            }
            
            charRect.width = size / 3;
            charRect.height = size;
            DrawRect(&buffer, &charRect, (Color){0, 255, 0});
            
            charRect.x = xStart;
            charRect.y = yStart;
        }
        
        SDL_UpdateTexture(texture, NULL, buffer.data, 4 * buffer.width);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }
    
    SDL_DestroyTexture(texture);
    free(buffer.data);
    free(bitmap);
    buffer.data = NULL;
    
    return 0;
}