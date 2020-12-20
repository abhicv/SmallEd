#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Windows.h>

#include <SDL2/SDL.h>

#include "types.h"

#include "smalled_font.c"
#include "smalled_text.c"
#include "smalled_file.c"
#include "smalled_render.c"

#include "smalled_lexer.h"
#include "smalled_memory.h"

#define DEFAULT_SCREEN_WIDTH 1280
#define DEFAULT_SCREEN_HEIGHT 720

#define MAX_SCREEN_WIDTH 1980
#define MAX_SCREEN_HEIGHT 1080

typedef struct EditSpace
{
    TextSequence textSeq;
    
    u8 loadedFileName[250];
    
    u32 nLines;
    u32 currentLine;
    u32 currentColumn;
    
    u32 lowestLine;
    u32 maxLinesDisplayable;
    
    Rect caretRect;
    Rect copyCaretRect;
    Rect editAreaRect;
    Rect lineMarginRect;
    Rect headerRect;
    
} EditSpace;

void EditSpaceEventHandler(SDL_Event *event, EditSpace *editSpace)
{
    
}

void SetEditSpaceDimensions(EditSpace *editSpace, u32 x, u32 y, u32 width, u32 height)
{
    
}

void RenderEditSpace(Buffer *renderBuffer, EditSpace *editSpace)
{
    
}

int main(int argc, char *argv[])
{
    if(SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        SDL_Log("Failed to initialize SDL : %s\n", SDL_GetError());
        return 1;
    }
    
    SDL_Window *window = SDL_CreateWindow("SmallEd", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, DEFAULT_SCREEN_WIDTH, DEFAULT_SCREEN_HEIGHT,  SDL_WINDOW_RESIZABLE);
    
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
    
    SDL_Rect displayBounRect = {0};
    SDL_GetDisplayBounds(SDL_GetWindowDisplayIndex(window), &displayBounRect);
    
    //allocated by SDL internally
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR32, SDL_TEXTUREACCESS_STATIC, DEFAULT_SCREEN_WIDTH, DEFAULT_SCREEN_HEIGHT);
    
    if(texture == NULL)
    {
        printf("Failed to create SDL texture : %s\n", SDL_GetError());
        return 1;
    }
    
    //all memory ever needed by the application
    Memory memory = {0};
    memory.permanentStorageSize = MegaByte(64);
    memory.permanentStorage = VirtualAlloc(0, memory.permanentStorageSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    
    //pixel buffer
    Buffer displayBuffer = {0};
    
    //allocating maximum size display buffer
    displayBuffer.data = AllocateMemory(&memory, sizeof(u32) * displayBounRect.w * displayBounRect.h);
    displayBuffer.width = DEFAULT_SCREEN_WIDTH;
    displayBuffer.height = DEFAULT_SCREEN_HEIGHT;
    
    FontData *fontData = LoadFont(&memory, "font/JetBrainsMono-Regular.ttf", 22);
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
    lineHighlight.width = DEFAULT_SCREEN_WIDTH;
    lineHighlight.height = caret.height;
    
    Rect header = {0};
    header.x = 0;
    header.y = 0;
    header.width = DEFAULT_SCREEN_WIDTH;
    header.height = fontData->lineGap + fontData->ascent - fontData->descent + 4;
    
    Rect lineMargin = {0};
    lineMargin.x = 0;
    lineMargin.y = header.x + header.height;
    lineMargin.width = 3 * roundf(a * fontData->scale) + 5;
    lineMargin.height = DEFAULT_SCREEN_HEIGHT;
    
    TextSequence textSeq = {0};
    textSeq.bufferCapacity = TEXT_BUFFER_SIZE;
    textSeq.buffer = AllocateMemory(&memory, TEXT_BUFFER_SIZE);
    
    //reading a file
    {
        char *fileName = "test_file.c";
        
        if(argc > 1)
        {
            fileName = argv[1];
        }
        
        //windows file api
        HANDLE fileHnd = NULL;
        fileHnd = CreateFileA(fileName,
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);
        
        if(fileHnd != INVALID_HANDLE_VALUE)
        {
            u32 size = 0;
            size = GetFileSize(fileHnd, NULL);
            
            u32 readBytes = 0;
            ReadFile(fileHnd,
                     &textSeq.buffer[TEXT_BUFFER_SIZE - size],
                     size,
                     &readBytes,
                     NULL);
            
            if(readBytes == size)
            {
                printf("Read all bytes from '%s'\n", fileName);
            }
            else
            {
                printf("error reading bytes from '%s'\n", fileName);
            }
            
            CloseHandle(fileHnd);
            
            textSeq.gapSize = textSeq.bufferCapacity - size;
            textSeq.preSize = 0;
            textSeq.postSize = size;
        }
        else
        {
            printf("failed to open the file '%s'\n", fileName);
            return 1;
        }
    }
    
    u32 currentActiveLineNumber = 1;
    u32 maxLinesDisplayable = (DEFAULT_SCREEN_HEIGHT - header.height) / caret.height;
    u32 lowestLineNumber = 1;
    u32 numOfLines = 0;
    
    i32 copyStartIndex = 0;
    
    b32 quit = false;
    
    SDL_Event event = {0};
    
    SDL_StartTextInput();
    while(!quit)
    {
        while(SDL_PollEvent(&event) > 0)
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
                    MoveCursorLeft(&textSeq);
                }
                else if(event.key.keysym.sym == SDLK_RIGHT)
                {
                    MoveCursorRight(&textSeq);
                }
                else if(event.key.keysym.sym == SDLK_UP)
                {
                    MoveCursorUp(&textSeq);
                }
                else if(event.key.keysym.sym == SDLK_DOWN)
                {
                    MoveCursorDown(&textSeq);
                }
#if 0
                else if(event.key.keysym.sym == SDLK_LALT)
                {
                    copyStartIndex = textSeq.preEndIndex;
                }
                else if(event.key.keysym.sym == SDLK_c && ctrlDown)
                {
                    printf("copy\n");
                    if(textSeq.preEndIndex > copyStartIndex)
                    {
                        u32 size = textSeq.preEndIndex - copyStartIndex;
                        u8 *copiedText = (u8*)malloc(size + 1); //+1 for null character
                        
                        for(u32 n = 0; n < size; n++)
                        {
                            copiedText[n] = textSeq.buffer[n + copyStartIndex + 1];
                        }
                        copiedText[size] = 0;
                        
                        printf("copied:\n'%s'\n", copiedText);
                        SDL_SetClipboardText(copiedText);
                        free(copiedText);
                    }
                    else
                    {
                        u32 size = copyStartIndex - textSeq.preEndIndex;
                        u8 *copiedText = (u8*)malloc(size + 1); //+1 for null character
                        
                        for(u32 n = 0; n < size; n++)
                        {
                            copiedText[n] = textSeq.buffer[n + textSeq.preSize];
                        }
                        copiedText[size] = 0;
                        
                        printf("copied:\n'%s'\n", copiedText);
                        SDL_SetClipboardText(copiedText);
                        free(copiedText);
                    }
                }
                else if(event.key.keysym.sym == SDLK_v && ctrlDown)
                {
                    printf("paste\n");
                    u8 *textToPaste = SDL_GetClipboardText();
                    
                    u32 n = 0;
                    while(textToPaste[n] != 0)
                    {
                        InsertItem(&textSeq, textToPaste[n]);
                        n++;
                    }
                }
                else if(event.key.keysym.sym == SDLK_x && ctrlDown)
                {
                    printf("cut\n");
                }
                else if(event.key.keysym.sym == SDLK_o && ctrlDown)
                {
                    printf("open file\n");
                }
                else if(event.key.keysym.sym == SDLK_s && ctrlDown)
                {
                    printf("save file\n");
                }
                else if(event.key.keysym.sym == SDLK_n && ctrlDown)
                {
                    printf("new file\n");
                }
#endif
                break;
                case SDL_TEXTINPUT:
                InsertItem(&textSeq, event.text.text[0]);
                //printf("typed: %s\n", event.text.text);
                break;
                
                case SDL_WINDOWEVENT:
                if((event.window.event == SDL_WINDOWEVENT_RESIZED) || (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED))
                {
                    
                    i32 w, h;
                    SDL_GetWindowSize(window, &w, &h);
                    printf("Window size changed to width : %d, height : %d\n", w, h);
                    
                    //resizing display buffer and SDL_Texture to fit to new window dimensions
                    SDL_DestroyTexture(texture);
                    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR32, SDL_TEXTUREACCESS_STATIC, w, h);
                    
                    displayBuffer.width = w;
                    displayBuffer.height = h;
                }
                break;
            }
        }
        
        ClearBuffer(&displayBuffer, (Color){9, 13, 18, 255});
        
        //NOTE(abhicv): calculating caret x position, active line number, total no. of lines
        {
            caret.x = lineMargin.width;
            caret.y = 0;
            currentActiveLineNumber = 1;
            numOfLines = 0;
            
            for(u32 n = 0; n < textSeq.preSize; n++)
            {
                if(textSeq.buffer[n] == '\n')
                {
                    caret.x = lineMargin.width;
                    currentActiveLineNumber++;
                    numOfLines++;
                    caret.y += caret.height;
                }
                else if(textSeq.buffer[n] == '\t')
                {
                    i32 advance, lsb;
                    stbtt_GetCodepointHMetrics(&fontData->fontInfo, '\t', &advance, &lsb);
                    caret.x += 4 * roundf(advance * fontData->scale);
                }
                else
                {
                    i32 advance, lsb;
                    stbtt_GetCodepointHMetrics(&fontData->fontInfo, textSeq.buffer[n], &advance, &lsb);
                    caret.x += roundf(advance * fontData->scale);
                    caret.width = roundf(advance * fontData->scale);
                }
                
                if(n == copyStartIndex)
                {
                    copyPasteCaret.x = caret.x;
                    copyPasteCaret.y = caret.y;
                    copyPasteCaret.width = caret.width;
                }
            }
            
            for(u32 n = (textSeq.bufferCapacity - textSeq.postSize); n < textSeq.bufferCapacity - 1; n++)
            {
                if(textSeq.buffer[n] == '\n')
                {
                    numOfLines++;
                }
            }
            numOfLines++;
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
        
        //caret y position
        caret.y = lineMargin.y + (currentActiveLineNumber - lowestLineNumber) * caret.height;
        lineHighlight.y = caret.y;
        
        //Rendering
        {
            //header
            DrawRect(&displayBuffer, &header, (Color){49, 94, 104, 255});
            
            //line margins
            DrawRect(&displayBuffer, &lineMargin, (Color){26, 38, 52, 255});
            
            //line highlight
            DrawRect(&displayBuffer, &lineHighlight, (Color){26, 38, 52, 255});
            
            //caret
            DrawRect(&displayBuffer, &caret, (Color){238, 232, 0, 255});
            
            //copy paste caret
            DrawRectWire(&displayBuffer, &copyPasteCaret, (Color){255, 255, 255, 255});
            
            //NOTE(abhicv): line numbers
            {
                u32 p = lineMargin.y;
                for(u32 n = lowestLineNumber; (n <= (maxLinesDisplayable + lowestLineNumber)) && (n <= numOfLines); n++)
                {
                    char number[3] = {0, 0, 0};
                    sprintf(&number[0], "%d\0", n);
                    RenderText(&displayBuffer, &number[0], fontData, fontBitMaps, lineMargin.x + 1, p, 0, 2);
                    p += fontData->lineGap + fontData->ascent - fontData->descent;
                }
            }
            
            u32 startIndex = 0;
            u32 endIndex = 0;
            
            u32 highestLineNumber = lowestLineNumber + maxLinesDisplayable;
            
            //NOTE(abhicv): calculating start and end index from lowestLineNumber and highestLineNumber
            {
                u32 cl = currentActiveLineNumber;
                
                if(textSeq.preSize > 0)
                {
                    for(u32 n = textSeq.preSize - 1; n > 0; n--)
                    {
                        if(textSeq.buffer[n] == '\n')
                        {
                            cl--;
                            if(cl == (lowestLineNumber - 1))
                            {
                                startIndex = n + 1;
                                break;
                            }
                        }
                    }
                }
                
                cl = currentActiveLineNumber - 1;
                
                if(textSeq.postSize > 0)
                {
                    for(u32 n = (textSeq.bufferCapacity - textSeq.postSize); n < textSeq.bufferCapacity; n++)
                    {
                        if(textSeq.buffer[n] == '\n' || (n == (textSeq.bufferCapacity - 1)))
                        {
                            cl++;
                            if(cl == highestLineNumber || cl == numOfLines)
                            {
                                endIndex = n;
                                break;
                            }
                        }
                    }
                }
            }
            
            //printf("si: %d, ei: %d, pre: %d, post: %d, ll: %d, hl: %d, nl: %d\n", startIndex, endIndex, textSeq.preSize, textSeq.postSize, lowestLineNumber, highestLineNumber, numOfLines);
            
            //syntax highlighting
            u32 totalSize = textSeq.preSize + textSeq.postSize;
            
            u8* colorIndexBuffer = (u8*)malloc(totalSize);
            memset(colorIndexBuffer, 3, totalSize);
            
            u8* textBuffer = (u8*)malloc(totalSize);
            memset(textBuffer, 0, totalSize);
            
            memcpy(textBuffer, textSeq.buffer, textSeq.preSize);
            
            if(textSeq.postSize > 0)
            {
                memcpy(textBuffer + textSeq.preSize, textSeq.buffer + (textSeq.bufferCapacity - textSeq.postSize), textSeq.postSize);
            }
            
            //Lexical analysis to fint ketwords and strings and fill colorIndexbuffer
            Lexer(textBuffer, totalSize, colorIndexBuffer, totalSize);
            
            RenderTextBuffer(&displayBuffer, &textSeq, fontData, fontBitMaps, colorIndexBuffer, lineMargin.width, lineMargin.y, startIndex, endIndex);
            
            free(colorIndexBuffer);
            free(textBuffer);
            
            SDL_UpdateTexture(texture, NULL, displayBuffer.data, 4 * displayBuffer.width);
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);
        }
    }
    SDL_DestroyTexture(texture);
    
    SDL_Quit();
    return 0;
}
