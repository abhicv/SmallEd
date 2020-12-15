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

enum EditorState 
{
    EDITOR_VIEW,
    DIRECTORY_LISTING_VIEW,
    COMMAND_LISTING_VIEW,
};

typedef struct TextEditSpace
{
    TextSequence textSeq;
    
    u32 activeLineNumber;
    u32 activeColumnNmumber;
    u32 lowestLineNumber;
    u32 maxLinesDisplayable;
    
    Rect editSpace;
    Rect caret;
    Rect copyPasteCaret;
    Rect lineHighlight;
    
    u8 *clipBoardBuffer;
    
    b32 enableLineNumbers;
    
    Color bgColor;
    Color textColor;
    Color lineHighlightColor;
    Color caretColor;
    
} TextEditSpace;

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
    
    FontData fontData = LoadFont("font/JetBrainsMono-Regular.ttf", 21);
    
    FontBitMap fontBitMaps[256];
    
    //NOTE(abhicv): ascii character bitmaps loading
    {
        for(u8 n = 33; n < 127; n++)
        {
            //Bitmap Rasterizing
            fontBitMaps[n].bitMap = stbtt_GetCodepointBitmap(&fontData.fontInfo, 0, fontData.scale, n, &fontBitMaps[n].width, &fontBitMaps[n].height, &fontBitMaps[n].xOffset, &fontBitMaps[n].yOffset);
        }
    }
    
    Rect caret = {0};
    caret.x = 0;
    caret.y = 0;
    caret.width = 4;
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
    
    i32 a = 0, l = 0;
    stbtt_GetCodepointHMetrics(&fontData.fontInfo, '8', &a, &l);
    
    Rect lineMargin = {0};
    lineMargin.x = 0;
    lineMargin.y = 0;
    lineMargin.width = 3 * roundf(a * fontData.scale) + 5;
    lineMargin.height = SCREEN_HEIGHT;
    
    TextSequence textSeq = {0};
    textSeq.buffer = (u8*)malloc(TEXT_BUFFER_SIZE);
    memset(&textSeq.buffer[0], 0, TEXT_BUFFER_SIZE);
    
    //reading a file
    {
        char *file_name = "hello.c";
        
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
            u32 status = fread(&textSeq.buffer[TEXT_BUFFER_SIZE - size], 1, size, file);
            
            if(status == size)
            {
                printf("Read all bytes successfully from '%s'\n", file_name);
            }
            else
            {
                printf("error reading bytes from '%s'\n", file_name);
            }
            
            fclose(file);
            
            textSeq.gapSize = TEXT_BUFFER_SIZE - size;
            textSeq.preEndIndex = -1;
            textSeq.postStartIndex = TEXT_BUFFER_SIZE - size;
            textSeq.bufferSize = TEXT_BUFFER_SIZE;
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
    u32 numOfLines = 0;
    
    i32 copyStartIndex = 0;
    
    b32 quit = false;
    
    SDL_Event event = {0};
    
    SDL_StartTextInput();
    while(!quit)
    {
        if(SDL_WaitEvent(&event) > 0)
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
                            copiedText[n] = textSeq.buffer[n + textSeq.preEndIndex + 1];
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
        
        ClearBuffer(&displayBuffer, (Color){20, 20, 20, 255});
        
        //NOTE(abhicv): calculating caret x position, active line number, total no. of lines
        {
            caret.x = lineMargin.width;
            caret.y = 0;
            currentActiveLineNumber = 1;
            numOfLines = 0;
            
            for(i32 n = 0; n <= textSeq.preEndIndex; n++)
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
                    stbtt_GetCodepointHMetrics(&fontData.fontInfo, '\t', &advance, &lsb);
                    caret.x += 4 * roundf(advance * fontData.scale);
                }
                else
                {
                    i32 advance, lsb;
                    stbtt_GetCodepointHMetrics(&fontData.fontInfo, textSeq.buffer[n], &advance, &lsb);
                    caret.x += roundf(advance * fontData.scale);
                    caret.width = roundf(advance * fontData.scale);
                }
                
                if(n == copyStartIndex)
                {
                    copyPasteCaret.x = caret.x;
                    copyPasteCaret.y = caret.y;
                    copyPasteCaret.width = caret.width;
                }
            }
            
            for(i32 n = textSeq.postStartIndex; n < (TEXT_BUFFER_SIZE - 1); n++)
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
        caret.y = (currentActiveLineNumber - lowestLineNumber) * caret.height;
        lineHighlight.y = caret.y;
        
        //Rendering
        {
            //line margin
            DrawRectWire(&displayBuffer, &lineMargin, (Color){50, 50, 50, 255});
            
            //line highlight
            DrawRect(&displayBuffer, &lineHighlight, (Color){40, 40, 40, 255});
            
            //caret
            DrawRect(&displayBuffer, &caret, (Color){255, 255, 0, 255});
            
            //copy paste caret
            DrawRectWire(&displayBuffer, &copyPasteCaret, (Color){255, 255, 255, 255});
            
            //NOTE(abhicv): line numbers
            {
                u32 p = lineMargin.y;
                for(u32 n = lowestLineNumber; (n <= (maxLinesDisplayable + lowestLineNumber)) && (n <= numOfLines); n++)
                {
                    char number[3] = {0, 0, 0};
                    sprintf(&number[0], "%d\0", n);
                    RenderText(&displayBuffer, &number[0], &fontData, fontBitMaps, lineMargin.x + 1, p, 0, 2);
                    p += fontData.lineGap + fontData.ascent - fontData.descent;
                }
            }
            
            u32 startIndex = 0;
            u32 endIndex = 0;
            
            u32 highestLineNumber = lowestLineNumber + maxLinesDisplayable;
            
            //NOTE(abhicv): calculating start and end index from lowestLineNumber and highestLineNumber
            {
                u32 cl = currentActiveLineNumber;
                for(i32 n = textSeq.preEndIndex; n >= 0; n--)
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
                
                cl = currentActiveLineNumber - 1;
                
                for(i32 n = textSeq.postStartIndex; n < TEXT_BUFFER_SIZE; n++)
                {
                    if(textSeq.buffer[n] == '\n' || (n == (TEXT_BUFFER_SIZE - 1)))
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
            
            //printf("si: %d, ei: %d, pre: %d, post: %d, ll: %d, hl: %d, nl: %d\n", startIndex, endIndex, textSeq.preEndIndex, textSeq.postStartIndex, lowestLineNumber, highestLineNumber, numOfLines);
            
            //syntax highlighting
            u32 size = (textSeq.preEndIndex + 1) + (TEXT_BUFFER_SIZE - textSeq.postStartIndex);
            u8* colorIndexBuffer = (u8*)malloc(size);
            memset(colorIndexBuffer, 3, size);
            
            u8* textBuffer = (u8*)malloc(size);
            memset(textBuffer, 0, size);
            memcpy(textBuffer, textSeq.buffer, textSeq.preEndIndex + 1);
            memcpy(textBuffer + textSeq.preEndIndex + 1, textSeq.buffer + textSeq.postStartIndex, (TEXT_BUFFER_SIZE - textSeq.postStartIndex));
            
            //Lexical analysis to fint ketwords and strings and fill colorIndexbuffer
            Lexer(textBuffer, size, colorIndexBuffer, size);
            
            RenderTextBuffer(&displayBuffer, &textSeq.buffer[0], &fontData, fontBitMaps, colorIndexBuffer, lineMargin.width, 0, textSeq.preEndIndex, textSeq.postStartIndex, startIndex, endIndex);
            
            free(colorIndexBuffer);
            free(textBuffer);
            
            SDL_UpdateTexture(texture, NULL, displayBuffer.data, 4 * displayBuffer.width);
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);
        }
    }
    
    free(displayBuffer.data);
    displayBuffer.data = NULL;
    
    free(textSeq.buffer);
    textSeq.buffer = NULL;
    
    SDL_Quit();
    return 0;
}
