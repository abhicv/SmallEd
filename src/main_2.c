#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL2/SDL.h>

#include "types.h"
#include "font.c"
#include "text.c"
#include "file.c"
#include "render.c"
#include "lexer.h"

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

//now one text sequence for entire editing session(a file)
//seeking cursor through large file is slow

//solution thought of
struct EditorSpace
{
    // each text sequence is a line
    // each line can have upto 1024 ascii characters, if need more can be allocated on demand
    // memory allocations are to be kept minimum
    // 10 * 1024 lines max = 10240
    // Total buffer size = 10 * 1024 * 1020 = 10 MB = 10485760 bytes or ascii characters
    
    TextSequence *lines[10 * 1024];
    TextSequence *currentTextSequence;
    
    //when adding new lines all previs lines are to be shifted down the line
    //since here lines are pointers to text sequence.
    //or gap buffer of TextSequence pointers can also be made,so inserting new lines become trivial
    
}EditorSpace;

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
    
    //pixel buffer
    Buffer displayBuffer = {0};
    displayBuffer.data = (u32*)malloc(sizeof(u32) * SCREEN_WIDTH * SCREEN_HEIGHT);
    displayBuffer.width = SCREEN_WIDTH;
    displayBuffer.height = SCREEN_HEIGHT;
    
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR32, SDL_TEXTUREACCESS_STATIC, SCREEN_WIDTH, SCREEN_HEIGHT);
    if(texture == NULL)
    {
        printf("Failed to create SDL texture : %s\n", SDL_GetError());
    }
    
    FontData fontData = LoadFont("font/JetBrainsMono-Regular.ttf", 20);
    
    FontBitMap fontBitMaps[256];
    
    //NOTE(abhicv): ascii character bitmaps loading
    {
        for(u8 n = 33; n < 127; n++)
        {
            //Bitmap Rasterizing
            fontBitMaps[n].bitMap = stbtt_GetCodepointBitmap(&fontData.fontInfo, 0, fontData.scale, n, &fontBitMaps[n].width, &fontBitMaps[n].height, &fontBitMaps[n].xOffset, &fontBitMaps[n].yOffset);
        }
    }
    
    i32 a = 0, l = 0;
    stbtt_GetCodepointHMetrics(&fontData.fontInfo, '8', &a, &l);
    
    Rect caret = {0};
    caret.x = 0;
    caret.y = 0;
    caret.width = roundf(a * fontData.scale);
    caret.height = fontData.lineGap + fontData.ascent - fontData.descent;
    
    Rect copyPasteCaret = {0};
    copyPasteCaret.x = 0;
    copyPasteCaret.y = 0;
    copyPasteCaret.width = 4;
    copyPasteCaret.height = fontData.lineGap + fontData.ascent - fontData.descent;
    
    Rect lineHighlight = {0};
    lineHighlight.x = 0;
    lineHighlight.y = 0;
    lineHighlight.width = SCREEN_WIDTH;
    lineHighlight.height = caret.height;
    
    Rect lineMargin = {0};
    lineMargin.x = 0;
    lineMargin.y = 0;
    lineMargin.width = 3 * roundf(a * fontData.scale) + 5;
    lineMargin.height = SCREEN_HEIGHT;
    
#define MAX_LINES 10 * 1024
#define MAX_CHARACTER_PER_LINE  1024
    
    TextSequence *lines[MAX_LINES] = {0};
    TextSequence *currentTextSequence = 0;
    
    for(u32 n = 0; n < MAX_LINES; n++)
    {
        TextSequence *tSeq = (TextSequence*)malloc(sizeof(TextSequence));
        //tSeq->buffer = (u8*)malloc(MAX_CHARACTER_PER_LINE);
        //memset(tSeq->buffer, 0, MAX_CHARACTER_PER_LINE);
        //tSeq->bufferCapacity = MAX_CHARACTER_PER_LINE;
        lines[n] = tSeq;
    }
    
    currentTextSequence = lines[0];
    u32 nLines = 0;
    
    //reading a file and breaking into lines
    {
        char *file_name = "test_file.c";
        
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
            
            //breaking file data into line TextSequences
            {
                u32 nCharInLine = 0;
                u32 lineStartIndex = 0;
                
                for(u32 n = 0; n < size; n++)
                {
                    if((fileData[n] == '\n') || (n == size - 1))
                    {
                        TextSequence *tSeq = lines[nLines];
                        
                        if(nCharInLine <= (MAX_CHARACTER_PER_LINE / 2))
                        {
                            tSeq->bufferCapacity = MAX_CHARACTER_PER_LINE;
                        }
                        else
                        {
                            tSeq->bufferCapacity = nCharInLine;
                        }
                        tSeq->buffer = (u8*)malloc(tSeq->bufferCapacity);
                        
                        tSeq->preSize = 0;
                        tSeq->postSize = nCharInLine;
                        tSeq->gapSize = tSeq->bufferCapacity - tSeq->postSize;
                        
                        memcpy(tSeq->buffer + (tSeq->bufferCapacity - tSeq->postSize), fileData + lineStartIndex, nCharInLine);
                        
                        //printf("nLines: %d, nCharInLine: %d, lineStartIndex: %d\n", nLines, nCharInLine, lineStartIndex);
                        nCharInLine = 0;
                        lineStartIndex = n + 1;
                        nLines++;
                    }
                    else
                    {
                        nCharInLine++;
                    }
                }
                //printf("numLines: %d\n", nLines);
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
                    DeleteItem(currentTextSequence);
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
                    MoveCursorLeft(currentTextSequence);
                    if(currentTextSequence->preSize == 0 && currentActiveLineNumber >= 2)
                    {
                        printf("preSize: %d, l: %d\n", currentTextSequence->preSize, currentActiveLineNumber);
                        currentTextSequence = lines[currentActiveLineNumber - 2];
                        
                        currentActiveLineNumber--;
                    }
                }
                else if(event.key.keysym.sym == SDLK_RIGHT)
                {
                    MoveCursorRight(currentTextSequence);
                }
                else if(event.key.keysym.sym == SDLK_UP)
                {
                    if(currentActiveLineNumber > 1)
                    {
                        currentActiveLineNumber--;
                        currentTextSequence = lines[currentActiveLineNumber - 1];
                    }
                }
                else if(event.key.keysym.sym == SDLK_DOWN)
                {
                    if(currentActiveLineNumber < nLines)
                    {
                        currentActiveLineNumber++;
                        currentTextSequence = lines[currentActiveLineNumber - 1];
                    }
                }
                break;
                
                case SDL_TEXTINPUT:
                InsertItem(currentTextSequence, event.text.text[0]);
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
                    
                    free(displayBuffer.data);
                    displayBuffer.data = (u32*)malloc(sizeof(u32) * w * h);
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
        
        //caret x position
        {
            caret.x = lineMargin.width;
            for(u32 n = 0; n < currentTextSequence->preSize; n++)
            {
                u8 c = currentTextSequence->buffer[n];
                
                i32 advance = 0, lsb = 0;
                stbtt_GetCodepointHMetrics(&fontData.fontInfo, c, &advance, &lsb);
                
                if(c == '\t') //tab
                {
                    caret.x += 4 * roundf(advance * fontData.scale);
                }
                else if(c >= '!' && c <= '~' || c == ' ')
                {
                    caret.x += roundf(advance * fontData.scale);
                }
            }
        }
        
        //caret y position
        caret.y = (currentActiveLineNumber - lowestLineNumber) * caret.height;
        lineHighlight.y = caret.y;
        
        //Rendering
        {
            ClearBuffer(&displayBuffer, (Color){9, 13, 18, 255});
            
            //line margin
            DrawRectWire(&displayBuffer, &lineMargin, (Color){50, 50, 50, 255});
            
            //line highlight
            DrawRect(&displayBuffer, &lineHighlight, (Color){26, 38, 52, 255});
            
            //caret
            DrawRect(&displayBuffer, &caret, (Color){238, 232, 0, 255});
            
            //NOTE(abhicv): line numbers
            {
                u32 p = lineMargin.y;
                for(u32 n = lowestLineNumber; (n <= (maxLinesDisplayable + lowestLineNumber)) && (n <= nLines); n++)
                {
                    char number[3] = {0, 0, 0};
                    sprintf(&number[0], "%d\0", n);
                    RenderText(&displayBuffer, &number[0], &fontData, fontBitMaps, lineMargin.x + 1, p, 0, 2);
                    p += fontData.lineGap + fontData.ascent - fontData.descent;
                }
            }
            
            u32 highestLineNumber = lowestLineNumber + maxLinesDisplayable;
            
            //printf("cl: %d, ll: %d, hl: %d, nl: %d\n", currentActiveLineNumber, lowestLineNumber, highestLineNumber, nLines);
            
            u32 y = 0;
            for(u32 n = lowestLineNumber; n <= highestLineNumber; n++)
            {
                RenderTextSequence(&displayBuffer, lines[n - 1], &fontData, fontBitMaps, lineMargin.width, y);
                y += fontData.lineGap + fontData.ascent - fontData.descent;
            }
            
            SDL_UpdateTexture(texture, NULL, displayBuffer.data, 4 * displayBuffer.width);
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);
        }
    }
    
    free(displayBuffer.data);
    displayBuffer.data = NULL;
    
    SDL_Quit();
    return 0;
}
