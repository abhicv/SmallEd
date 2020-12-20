#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL2/SDL.h>

#include "Windows.h"

#include "types.h"

#include "smalled_font.c"
#include "smalled_text.c"
#include "smalled_file.c"
#include "smalled_render.c"
#include "smalled_lexer.h"
#include "smalled_memory.h"

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

/*

in smalled_app.c now one text sequence for entire editing session(a file)
seeking cursor through large file is slow

Solution thought of:
each text sequence is a line
each line can have upto 1024 ascii characters, if need more can be allocated on demand
memory allocations are to be kept minimum
10 * 1024 lines max = 10240
Total buffer size = 10 * 1024 * 1020 = 10 MB = 10485760 bytes or ascii characters
when adding new lines all previs lines are to be shifted down the line
since here lines are pointers to text sequence.
or gap buffer of TextSequence pointers can also be made,so inserting new lines become trivial

*/

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
    
    SDL_Renderer *renderer = SDL_CreateRenderer(window, 3, SDL_RENDERER_ACCELERATED);
    
    if(renderer == NULL)
    {
        SDL_Log("Failed to create SDL renderer : %s\n", SDL_GetError());
        return 1;
    }
    
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR32, SDL_TEXTUREACCESS_STATIC, SCREEN_WIDTH, SCREEN_HEIGHT);
    if(texture == NULL)
    {
        printf("Failed to create SDL texture : %s\n", SDL_GetError());
        return 1;
    }
    
    SDL_Rect displayBounRect = {0};
    SDL_GetDisplayBounds(SDL_GetWindowDisplayIndex(window), &displayBounRect);
    
    Memory memory = {0};
    memory.permanentStorageSize = MegaByte(300);
    memory.permanentStorage = VirtualAlloc(0, memory.permanentStorageSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    
    //pixel buffer
    Buffer displayBuffer = {0};
    displayBuffer.data = (u32*)AllocateMemory(&memory, sizeof(u32) * displayBounRect.w * displayBounRect.h);
    displayBuffer.width = SCREEN_WIDTH;
    displayBuffer.height = SCREEN_HEIGHT;
    
    FontData *fontData = LoadFont(&memory, "font/JetBrainsMono-Regular.ttf", 20);
    
    FontBitMap *fontBitMaps = (FontBitMap*)AllocateMemory(&memory, sizeof(FontBitMap) * 256);
    
    //NOTE(abhicv): ascii character bitmaps loading
    {
        for(u8 n = 33; n < 127; n++)
        {
            //Bitmap Rasterizing
            fontBitMaps[n].bitMap = stbtt_GetCodepointBitmap(&fontData->fontInfo, 0, fontData->scale, n, &fontBitMaps[n].width, &fontBitMaps[n].height, &fontBitMaps[n].xOffset, &fontBitMaps[n].yOffset);
        }
    }
    
    i32 a = 0, l = 0;
    stbtt_GetCodepointHMetrics(&fontData->fontInfo, 'A', &a, &l);
    
    Rect caret = {0};
    caret.x = 0;
    caret.y = 0;
    caret.width = roundf(a * fontData->scale);
    caret.height = fontData->lineGap + fontData->ascent - fontData->descent;
    
    Rect copyPasteCaret = {0};
    copyPasteCaret.x = 0;
    copyPasteCaret.y = 0;
    copyPasteCaret.width = 4;
    copyPasteCaret.height = fontData->lineGap + fontData->ascent - fontData->descent;
    
    Rect lineHighlight = {0};
    lineHighlight.x = 0;
    lineHighlight.y = 0;
    lineHighlight.width = SCREEN_WIDTH;
    lineHighlight.height = caret.height;
    
    Rect lineMargin = {0};
    lineMargin.x = 0;
    lineMargin.y = 0;
    lineMargin.width = 3 * roundf(a * fontData->scale) + 5;
    lineMargin.height = SCREEN_HEIGHT;
    
#define MAX_LINES 50 * 1024
#define MIN_CHARACTER_PER_LINE  1024
    
    TextSequence *lines = (TextSequence*)AllocateMemory(&memory, sizeof(TextSequence) * MAX_LINES);
    TextSequence *currentTextSequence = &lines[0];
    u32 nLines = 0;
    
    //reading a file and breaking into lines
    {
        //char *file_name = "testfile_20MB";
        char *file_name = "../STB/stb_truetype.h";
        //char *file_name = "test_file.c";
        
        if(argc > 1)
        {
            file_name = argv[1];
        }
        
        FILE *file = fopen(file_name, "rb");
        if(file != NULL)
        {
            fseek(file, 0, SEEK_END);
            u32 size = ftell(file);
            fseek(file, 0, SEEK_SET);
            u8* fileData = (u8*)malloc(size);
            
            u32 status = fread(fileData, 1, size, file);
            
            if(status == size)
            {
                printf("Read all bytes successfully from '%s'\n", file_name);
            }
            else
            {
                printf("error reading bytes from '%s'\n", file_name);
            }
            fclose(file);
            
            //breaking file data into TextSequences
            {
                u32 nCharInLine = 0;
                u32 lineStartIndex = 0;
                
                for(u32 n = 0; n < size; n++)
                {
                    if((fileData[n] == '\n') || (n == size - 1))
                    {
                        TextSequence *tSeq = &lines[nLines];
                        
                        if(nCharInLine <= (MIN_CHARACTER_PER_LINE / 2))
                        {
                            tSeq->bufferCapacity = MIN_CHARACTER_PER_LINE;
                        }
                        else
                        {
                            tSeq->bufferCapacity = nCharInLine * 2;
                        }
                        
                        tSeq->buffer = (u8*)AllocateMemory(&memory, tSeq->bufferCapacity);
                        tSeq->colorIndexBuffer = (u8*)AllocateMemory(&memory, tSeq->bufferCapacity);
                        
                        tSeq->preSize = 0;
                        tSeq->postSize = nCharInLine;
                        tSeq->gapSize = tSeq->bufferCapacity - tSeq->postSize;
                        
                        memcpy(tSeq->buffer + (tSeq->bufferCapacity - tSeq->postSize), fileData + lineStartIndex, nCharInLine);
                        
                        nCharInLine = 0;
                        lineStartIndex = n + 1;
                        nLines++;
                    }
                    else
                    {
                        nCharInLine++;
                    }
                }
                free(fileData);
            }
        }
        else
        {
            printf("failed to open the file '%s'\n", file_name);
            return 1;
        }
    }
    
    u32 currentActiveLineNumber = 1;
    u32 maxLinesDisplayable = (SCREEN_HEIGHT) / caret.height;
    u32 lowestLineNumber = 1;
    
    b32 quit = false;
    
    u32 gotoLine = 0;
    
    SDL_Event event = {0};
    
    SDL_StartTextInput();
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
                
                b32 leftCtrlDown = (SDL_GetModState() & KMOD_LCTRL) == KMOD_LCTRL;
                b32 rightCtrlDown = (SDL_GetModState() & KMOD_RCTRL) == KMOD_RCTRL;
                b32 ctrlDown = (leftCtrlDown || rightCtrlDown);
                
                if(event.key.keysym.sym == SDLK_BACKSPACE)
                {
                    if(currentTextSequence->preSize == 0 && currentActiveLineNumber >= 2)
                    {
                        currentActiveLineNumber--;
                        currentTextSequence = &lines[currentActiveLineNumber - 1];
                        
                        while(currentTextSequence->postSize > 0)
                        {
                            MoveCursorRight(currentTextSequence);
                        }
                    }
                    else
                    {
                        DeleteItem(currentTextSequence);
                    }
                }
                else if(event.key.keysym.sym == SDLK_ESCAPE)
                {
                    quit = true;
                }
                else if(event.key.keysym.sym == SDLK_RETURN)
                {
                    InsertItem(currentTextSequence, '\n');
                }
                else if(event.key.keysym.sym == SDLK_TAB)
                {
                    InsertItem(currentTextSequence, '\t');
                }
                else if(event.key.keysym.sym == SDLK_LEFT)
                {
                    if(currentTextSequence->preSize == 0 && currentActiveLineNumber >= 2)
                    {
                        currentActiveLineNumber--;
                        currentTextSequence = &lines[currentActiveLineNumber - 1];
                        
                        while(currentTextSequence->postSize > 0)
                        {
                            MoveCursorRight(currentTextSequence);
                        }
                    }
                    else
                    {
                        MoveCursorLeft(currentTextSequence);
                    }
                }
                else if(event.key.keysym.sym == SDLK_RIGHT)
                {
                    if(currentTextSequence->postSize == 0 && currentActiveLineNumber < nLines)
                    {
                        currentActiveLineNumber++;
                        currentTextSequence = &lines[currentActiveLineNumber - 1];
                        
                        while(currentTextSequence->preSize > 0)
                        {
                            MoveCursorLeft(currentTextSequence);
                        }
                    }
                    else
                    {
                        MoveCursorRight(currentTextSequence);
                    }
                }
                else if(event.key.keysym.sym == SDLK_UP)
                {
                    if(currentActiveLineNumber > 1)
                    {
                        currentActiveLineNumber--;
                        currentTextSequence = &lines[currentActiveLineNumber - 1];
                    }
                }
                else if(event.key.keysym.sym == SDLK_DOWN)
                {
                    if(currentActiveLineNumber < nLines)
                    {
                        currentActiveLineNumber++;
                        currentTextSequence = &lines[currentActiveLineNumber - 1];
                    }
                }
                else if(leftCtrlDown)
                {
                    gotoLine += 1000;
                    currentTextSequence = &lines[gotoLine - 1];
                    currentActiveLineNumber = gotoLine;
                    lowestLineNumber = currentActiveLineNumber - (maxLinesDisplayable / 2);
                }
                else if(rightCtrlDown)
                {
                    gotoLine -= 1000;
                    if(gotoLine > 0)
                    {
                        currentTextSequence = &lines[gotoLine - 1];
                        currentActiveLineNumber = gotoLine;
                        lowestLineNumber = currentActiveLineNumber - (maxLinesDisplayable / 2);
                    }
                }
                
                break;
                
                case SDL_TEXTINPUT:
                InsertItem(currentTextSequence, event.text.text[0]);
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
                    
                    displayBuffer.width = w;
                    displayBuffer.height = h;
                    
                    lineMargin.height = h;
                    lineHighlight.width = w;
                    maxLinesDisplayable = h / caret.height;
                }
                break;
            }
        }
        
        //scrolling by changing lowest visible line number
        if(currentActiveLineNumber > (lowestLineNumber + maxLinesDisplayable - 1))
        {
            lowestLineNumber++;
        }
        else if(currentActiveLineNumber < lowestLineNumber)
        {
            lowestLineNumber--;
        }
        
        u32 number = currentActiveLineNumber;
        u32 nDigits = 0;
        while (number != 0) 
        {
            number /= 10;
            ++nDigits;
        }
        
        lineMargin.width = nDigits * roundf(a * fontData->scale) + 5;;
        
        //caret position
        {
            caret.x = lineMargin.width;
            for(u32 n = 0; n < currentTextSequence->preSize; n++)
            {
                u8 c = currentTextSequence->buffer[n];
                
                if(c == '\t') //tab
                {
                    i32 advance = 0, lsb = 0;
                    stbtt_GetCodepointHMetrics(&fontData->fontInfo, 'A', &advance, &lsb);
                    caret.x += 4 * roundf(advance * fontData->scale);
                }
                else if(c >= '!' && c <= '~')
                {
                    i32 advance = 0, lsb = 0;
                    stbtt_GetCodepointHMetrics(&fontData->fontInfo, c, &advance, &lsb);
                    caret.x += roundf(advance * fontData->scale);
                }
                else
                {
                    i32 advance = 0, lsb = 0;
                    stbtt_GetCodepointHMetrics(&fontData->fontInfo, 'A', &advance, &lsb);
                    caret.x += roundf(advance * fontData->scale);
                }
            }
            
            caret.y = (currentActiveLineNumber - lowestLineNumber) * caret.height;
            lineHighlight.y = caret.y;
            //}
            
            u32 highestLineNumber = (lowestLineNumber + maxLinesDisplayable) < nLines ? (lowestLineNumber + maxLinesDisplayable) : nLines;
            
            //printf("cl: %d, ll: %d, hl: %d, nl: %d, capacity: %d\n", currentActiveLineNumber, lowestLineNumber, highestLineNumber, nLines, currentTextSequence->bufferCapacity);
            
            //printf("pre: %d, post: %d, capacity: %d\n", currentTextSequence->preSize, currentTextSequence->postSize, currentTextSequence->bufferCapacity);
            
            //Rendering
            {
                ClearBuffer(&displayBuffer, (Color){9, 13, 18, 255});
                
                //line margin
                DrawRectWire(&displayBuffer, &lineMargin, (Color){50, 50, 50, 255});
                
                //line highlight
                DrawRect(&displayBuffer, &lineHighlight, (Color){26, 38, 52, 255});
                
                //caret
                DrawRect(&displayBuffer, &caret, (Color){238, 232, 0, 255});
                
                u32 y = 0;
                for(u32 n = lowestLineNumber; n <= highestLineNumber; n++)
                {
                    //line numbers
                    {
                        u32 num = n;
                        u32 count = 0;
                        while (num != 0) 
                        {
                            num /= 10;     // n = n/10
                            ++count;
                        }
                        
                        u8 *number = (u8*)malloc(count);
                        
                        sprintf(&number[0], "%d\0", n);
                        RenderText(&displayBuffer, &number[0], fontData, fontBitMaps, lineMargin.x + 1, y, 0, count - 1);
                    }
                    
                    TextSequence *tSeq = &lines[n - 1];
                    
                    u32 size = tSeq->preSize + tSeq->postSize;
                    u8* textBuffer = (u8*)malloc(size);
                    
                    memset(tSeq->colorIndexBuffer, 3, size);
                    
                    memcpy(textBuffer, tSeq->buffer, tSeq->preSize);
                    
                    if(tSeq->postSize > 0)
                    {
                        memcpy(textBuffer + tSeq->preSize, tSeq->buffer + (tSeq->bufferCapacity - tSeq->postSize), tSeq->postSize);
                    }
                    
                    Lexer(textBuffer, size, tSeq->colorIndexBuffer, size);
                    
                    b32 activeLine = (n == currentActiveLineNumber) ? true : false;
                    RenderTextSequence(&displayBuffer, tSeq, fontData, fontBitMaps, lineMargin.width, y, activeLine);
                    y += fontData->lineGap + fontData->ascent - fontData->descent;
                    
                    free(textBuffer);
                }
                
                SDL_UpdateTexture(texture, NULL, displayBuffer.data, 4 * displayBuffer.width);
                SDL_RenderClear(renderer);
                SDL_RenderCopy(renderer, texture, NULL, NULL);
                SDL_RenderPresent(renderer);
            }
        }
    }
    
    SDL_Quit();
    return 0;
}
