// NOTE(abhicv): Smalled is a text editor with less complexity and functionality
// A Text Sequence is a gap buffer of ascii characters.
// Each text sequence is a line.
// A text buffer is gap buffer of text sequences(array of lines).
// Each line can have upto 512 ascii characters, if need more can be allocated on demand(Memory allocations are to be kept minimum).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//for windowing, input and rendering
#include <SDL2/SDL.h>

//for windows file IO
#include <Windows.h>

//common typedefs
#include "sed_types.h"

#include "sed_font.c"
#include "sed_text.c"
#include "sed_file.c"
#include "sed_render.c"
#include "sed_lexer.c"
#include "sed_editor.c"

#include "sed_memory.h"
#include "sed_config.h"
#include "sed_util.h"

#define SCREEN_WIDTH 1600
#define SCREEN_HEIGHT 900

int main(int argc, char **argv)
{
    if(SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        SDL_Log("Failed to initialize SDL : %s\n", SDL_GetError());
        return 1;
    }
    
    SDL_Window *window = SDL_CreateWindow("SmallEd", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE);
    
    if(window == NULL)
    {
        SDL_Log("Failed to create SDL window : %s\n", SDL_GetError());
        return 1;
    }
    
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    
    if(renderer == NULL)
    {
        SDL_Log("Failed to create SDL renderer : %s\n", SDL_GetError());
        return 1;
    }
    
    SDL_Rect displayBounRect = {0};
    SDL_GetDisplayBounds(SDL_GetWindowDisplayIndex(window), &displayBounRect);
    
    //printf("w: %d, h: %d\n", displayBounRect.w, displayBounRect.h);
    
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
    
    if(texture == NULL)
    {
        SDL_Log("Failed to create SDL texture : %s\n", SDL_GetError());
        return 1;
    }
    
    InitGlobalMemoryArena();
    
    //render buffer
    Buffer renderBuffer = {0};
    renderBuffer.data = (u32*)AllocatePersistentMemory(sizeof(u32) * displayBounRect.w * displayBounRect.h);
    renderBuffer.width = SCREEN_WIDTH;
    renderBuffer.height = SCREEN_HEIGHT;
    
    FontData *fontData = LoadFont("font/JetBrainsMono-Regular.ttf", 20.0f);
    
    Editor editor = {0};
    editor.mode = EDITOR_MODE_ENTRY;
    
    //bounds for drawing 
    editor.rect.x = 0;
    editor.rect.y = 0;
    editor.rect.width = SCREEN_WIDTH;
    editor.rect.height = SCREEN_HEIGHT;
    
    //for goto line text edit box
    editor.gotoLineTSeq.bufferCapacity = 10;
    editor.gotoLineTSeq.buffer = (u8*)malloc(10);
    editor.gotoLineTSeq.preSize = 0;
    editor.gotoLineTSeq.postSize = 0;
    editor.gotoLineTSeq.gapSize = 10;
    
    GetCurrentDirectory(256, editor.fileList.currentDir);
    printf("Current directory: %s\n", editor.fileList.currentDir);
    
    b32 quit = false;
    SDL_Event event = {0};
    
    //app config
    AppConfig config = ParseAppConfigFile("smalled.config");
    
    while(!quit)
    {
        while(SDL_PollEvent(&event))
        {
            switch(event.type)
            {
                case SDL_QUIT:
                quit = true;
                break;
                
                case SDL_KEYDOWN:
                if(event.key.keysym.sym == SDLK_ESCAPE)
                {
                    quit = true;
                }
                break;
                
                case SDL_WINDOWEVENT:
                if((event.window.event == SDL_WINDOWEVENT_RESIZED) || (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED))
                {
                    u32 w = 0, h = 0;
                    SDL_GetWindowSize(window, &w, &h);
                    printf("Window size changed to width : %d, height : %d\n", w, h);
                    
                    //resizing display buffer and SDL_Texture to fit to new window dimensions
                    SDL_DestroyTexture(texture);
                    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, w, h);
                    
                    renderBuffer.width = w;
                    renderBuffer.height = h;
                    
                    editor.rect.width = w;
                    editor.rect.height = h;
                }
                break;
            }
            EditorSpaceEvent(&event, &editor);
        }
        
        ClearBuffer(&renderBuffer, (Color){4, 35, 40, 255});
        AppUpdateAndRender(&renderBuffer, fontData, &editor);
        
        SDL_UpdateTexture(texture, NULL, renderBuffer.data, 4 * renderBuffer.width);
        SDL_RenderClear(renderer);
        
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        
        ResetTransientMemory();
    }
    
    SDL_Quit();
    return 0;
}
