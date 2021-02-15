// NOTE(abhicv): Smalled is a text editor with less complexity and functionality
// A Text Sequence is a gap buffer of ascii characters.
// Each text sequence is a line.
// A text buffer is gap buffer of text sequences(array of lines).
// Each line can have upto 2 * 1024 ascii characters, if need more can be allocated on demand(Memory allocations are to be kept minimum).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//for windowing, input and rendering
#include <SDL2/SDL.h>

//for windows file IO
#include "Windows.h"

//common type defs
#include "types.h"

#include "smalled_font.c"
#include "smalled_text.c"
#include "smalled_file.c"
#include "smalled_render.c"
#include "smalled_lexer.c"
#include "smalled_editor.c"

#include "smalled_memory.h"

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

int main(int argc, char *argv[])
{
    if(SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        SDL_Log("Failed to initialize SDL : %s\n", SDL_GetError());
        return 1;
    }
    
    SDL_Window *window = SDL_CreateWindow("SmallEd", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT,  SDL_WINDOW_RESIZABLE);
    
    if(window == NULL)
    {
        SDL_Log("Failed to create SDL window : %s\n", SDL_GetError());
        return 1;
    }
    
    SDL_Renderer *renderer = SDL_CreateRenderer(window, 3, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    
    if(renderer == NULL)
    {
        SDL_Log("Failed to create SDL renderer : %s\n", SDL_GetError());
        return 1;
    }
    
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR32, SDL_TEXTUREACCESS_STATIC, SCREEN_WIDTH, SCREEN_HEIGHT);
    if(texture == NULL)
    {
        SDL_Log("Failed to create SDL texture : %s\n", SDL_GetError());
        return 1;
    }
    
    //memory for data living forever(upto end of the program)
    Memory memory = {0};
    memory.permanentStorageSize = MegaByte(64);
    memory.permanentStorage = VirtualAlloc(0, memory.permanentStorageSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    
    SDL_Rect displayBounRect = {0};
    SDL_GetDisplayBounds(SDL_GetWindowDisplayIndex(window), &displayBounRect);
    
    //render buffer
    Buffer renderBuffer = {0};
    renderBuffer.data = (u32*)AllocateMemory(&memory, sizeof(u32) * displayBounRect.w * displayBounRect.h);
    renderBuffer.width = SCREEN_WIDTH;
    renderBuffer.height = SCREEN_HEIGHT;
    
    FontData *fontData = LoadFont(&memory, "font/JetBrainsMono-Regular.ttf", 18.0f);
    
    Editor editor = {0};
    
    editor.rect.x = 0;
    editor.rect.y = 0;
    editor.rect.width = SCREEN_WIDTH;
    editor.rect.height = SCREEN_HEIGHT;
    
    editor.gotoLineTSeq.bufferCapacity = 120;
    editor.gotoLineTSeq.buffer = (u8*)malloc(120);
    editor.gotoLineTSeq.preSize = 0;
    editor.gotoLineTSeq.postSize = 0;
    editor.gotoLineTSeq.gapSize = 120;
    
    //reading a file and breaking into lines
    char *fileName = "E:/Development/SmallEd/STB/stb_truetype.h\0";
    //char *fileName = "checker.c\0";
    
    //file name as cmd line argument
    if(argc > 1)
    {
        fileName = argv[1];
    }
    
    File file = ReadFileFromDisk(fileName);
    
    if(file.loaded)
    {
        u32 nLines = GetLineCount(file.buffer, file.size);
        
        printf("line count: %d\n", nLines);
        
        TextBuffer textBuffer = {0};
        
        textBuffer.capacity = 2 * nLines;
        
        textBuffer.lines = (TextSequence*)malloc(sizeof(TextSequence) * textBuffer.capacity);
        textBuffer.lowestLine = 0;
        textBuffer.currentLine = 0;
        textBuffer.preSize = 0;
        textBuffer.postSize = nLines - 1;
        textBuffer.gapSize = textBuffer.capacity - nLines;
        
        BreakFileIntoLines(file.buffer, file.size, nLines, &textBuffer);
        editor.textBuffer = textBuffer;
        
        editor.fileName = fileName;
    }
    
    free(file.buffer);
    file.buffer = NULL;
    
    editor.mode = EDITOR_MODE_ENTRY;
    b32 quit = false;
    
    //for fps calculation
    u64 frames = 0;
    
    SDL_Event event = {0};
    while(!quit)
    {
        SDL_StartTextInput();
        while(SDL_PollEvent(&event))
        {
            switch(event.type)
            {
                case SDL_QUIT:
                quit = true;
                break;
                
                case SDL_WINDOWEVENT:
                if((event.window.event == SDL_WINDOWEVENT_RESIZED) || (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED))
                {
                    i32 w = 0, h = 0;
                    SDL_GetWindowSize(window, &w, &h);
                    printf("Window size changed to width : %d, height : %d\n", w, h);
                    
                    //resizing display buffer and SDL_Texture to fit to new window dimensions
                    SDL_DestroyTexture(texture);
                    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR32, SDL_TEXTUREACCESS_STATIC, w, h);
                    
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
        RenderSpace(&renderBuffer, fontData, &editor);
        
#if 1
        //DEBUG: fps meter
        u8 fpsText[10] = {0};
        sprintf(fpsText, "%0.0f fps\0", frames * 1000.0f / SDL_GetTicks());
        RenderText(&renderBuffer, fpsText, strlen(fpsText), fontData, renderBuffer.width - 100, 0, (Color){10, 10, 10, 255});
#endif
        
        SDL_UpdateTexture(texture, NULL, renderBuffer.data, 4 * renderBuffer.width);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        
        frames++;
    }
    
    SDL_Quit();
    return 0;
}
