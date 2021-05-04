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

In smalled_app.c now one text sequence for entire editing session(a file)
seeking cursor through large file is slow

PLAN OF ATTACK

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

typedef struct Entity
{
	u32 playerHealth;
	u32 playerID;
	u32 nWeapons;
}

typedef struct TextBuffer
{
    TextSequence *lines;
    
    u32 preSize;
    u32 postSize;
    u32 gapSize;
    u32 capacity;
    
    u32 currentLine;
    u32 lowestLine;
    u32 maxLinesVisible;
    
} TextBuffer;

void BreakFileIntoLines(u8 *fileBuffer, u32 fileSize, u32 nLines, TextBuffer *textBuffer)
{
    u32 nCharInLine = 0;
    u32 lineStartIndex = 0;
    u32 lineCount = 0;
    
    for(u32 n = 0; n < fileSize; n++)
    {
        if((fileBuffer[n] == '\n') || (n == fileSize - 1))
        {
            if(lineCount == 0)
            {
                TextSequence *tSeq = &textBuffer->lines[textBuffer->preSize];
                
                //tSeq->bufferCapacity = 2 * nCharInLine;
                tSeq->bufferCapacity = KiloByte(2);
                
                tSeq->buffer = (u8*)malloc(tSeq->bufferCapacity);
                tSeq->colorIndexBuffer = (u8*)malloc(tSeq->bufferCapacity);
                
                tSeq->preSize = 0;
                tSeq->postSize = nCharInLine;
                tSeq->gapSize = tSeq->bufferCapacity - tSeq->postSize;
                
                memcpy(&tSeq->buffer[tSeq->bufferCapacity - tSeq->postSize], &fileBuffer[lineStartIndex], nCharInLine);
                
                textBuffer->preSize++;
            }
            else
            {
                TextSequence *tSeq = &textBuffer->lines[textBuffer->capacity - textBuffer->postSize + lineCount - 1];
                
                //tSeq->bufferCapacity = 2 * nCharInLine;
                tSeq->bufferCapacity = KiloByte(2);
                
                tSeq->buffer = (u8*)malloc(tSeq->bufferCapacity);
                tSeq->colorIndexBuffer = (u8*)malloc(tSeq->bufferCapacity);
                
                tSeq->preSize = 0;
                tSeq->postSize = nCharInLine;
                tSeq->gapSize = tSeq->bufferCapacity - tSeq->postSize;
                
                memcpy(&tSeq->buffer[tSeq->bufferCapacity - tSeq->postSize], &fileBuffer[lineStartIndex], nCharInLine);
            }
            
            nCharInLine = 0;
            lineStartIndex = n + 1;
            lineCount++;
        }
        else
        {
            nCharInLine++;
        }
    }
}

void InsertLine(TextBuffer *textBuffer)
{
    TextSequence *tSeq = &textBuffer->lines[textBuffer->preSize];
    
    tSeq->bufferCapacity = KiloByte(2);
    
    tSeq->buffer = (u8*)malloc(tSeq->bufferCapacity);
    tSeq->colorIndexBuffer = (u8*)malloc(tSeq->bufferCapacity);
    
    tSeq->preSize = 0;
    tSeq->postSize = textBuffer->lines[textBuffer->currentLine].postSize;
    tSeq->gapSize = tSeq->bufferCapacity - tSeq->postSize;
    
    if(tSeq->postSize > 0)
    {
        memcpy(&tSeq->buffer[tSeq->bufferCapacity - tSeq->postSize],
               &textBuffer->lines[textBuffer->currentLine].buffer[textBuffer->lines[textBuffer->currentLine].bufferCapacity - tSeq->postSize],
               tSeq->postSize);
    }
    
    //shrinking previous line
    textBuffer->lines[textBuffer->currentLine].postSize = 0;
    
    textBuffer->preSize++;
    textBuffer->gapSize--;
    textBuffer->currentLine++;
}

void DeleteLine(TextBuffer *textBuffer)
{
    if(textBuffer->preSize > 1)
    {
        TextSequence *cTSeq = &textBuffer->lines[textBuffer->currentLine];
        TextSequence *tSeq = &textBuffer->lines[textBuffer->currentLine - 1];
        
        while(tSeq->postSize > 0)
        {
            MoveCursorRight(tSeq);
        }
        
        for(u32 n = 0; n < cTSeq->preSize; n++)
        {
            InsertItem(tSeq, cTSeq->buffer[n]);
        }
        
        if(cTSeq->postSize > 0)
        {
            for(u32 n = (cTSeq->bufferCapacity - cTSeq->postSize); n < cTSeq->bufferCapacity; n++)
            {
                InsertItem(tSeq, cTSeq->buffer[n]);
            }
        }
        
        for(u32 n = 0; n < (cTSeq->preSize + cTSeq->postSize); n++)
        {
            MoveCursorLeft(tSeq);
        }
        
        free(cTSeq->buffer);
        free(cTSeq->colorIndexBuffer);
        
        textBuffer->preSize--;
        textBuffer->currentLine--;
        textBuffer->gapSize++;
    }
}

void MoveCursorUpGlobal(TextBuffer *textBuffer)
{
    if(textBuffer->preSize > 1)
    {
        u32 fromIndex = textBuffer->preSize - 1;
        u32 toIndex = textBuffer->capacity - textBuffer->postSize - 1;
        
        textBuffer->lines[toIndex] = textBuffer->lines[fromIndex];
        textBuffer->preSize--;
        textBuffer->postSize++;
        textBuffer->currentLine--;
    }
}

void MoveCursorDownGlobal(TextBuffer *textBuffer)
{
    if(textBuffer->postSize > 0)
    {
        u32 fromIndex = textBuffer->capacity - textBuffer->postSize;
        u32 toIndex = textBuffer->preSize;
        
        textBuffer->lines[toIndex] = textBuffer->lines[fromIndex];
        textBuffer->preSize++;
        textBuffer->postSize--;
        textBuffer->currentLine++;
    }
}

void RenderTextBuffer(Buffer *renderBuffer)
{
    
}

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
        printf("Failed to create SDL texture : %s\n", SDL_GetError());
        return 1;
    }
    
    SDL_Rect displayBounRect = {0};
    SDL_GetDisplayBounds(SDL_GetWindowDisplayIndex(window), &displayBounRect);
    
    Memory memory = {0};
    memory.permanentStorageSize = MegaByte(64);
    memory.permanentStorage = VirtualAlloc(0, memory.permanentStorageSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    
    //pixel buffer
    Buffer displayBuffer = {0};
    displayBuffer.data = (u32*)AllocateMemory(&memory, sizeof(u32) * displayBounRect.w * displayBounRect.h);
    displayBuffer.width = SCREEN_WIDTH;
    displayBuffer.height = SCREEN_HEIGHT;
    
    FontData *fontData = LoadFont(&memory, "font/JetBrainsMono-Regular.ttf", 18);
    
    Rect caret = {0};
    caret.x = 0;
    caret.y = 0;
    caret.width = fontData->charDatas[(u32)('A')].xadvance;
    caret.height = fontData->lineHeight;
    
    Rect lineHighlight = {0};
    lineHighlight.x = 0;
    lineHighlight.y = 0;
    lineHighlight.width = SCREEN_WIDTH;
    lineHighlight.height = caret.height;
    
    Rect lineMargin = {0};
    lineMargin.x = 0;
    lineMargin.y = 0;
    lineMargin.height = SCREEN_HEIGHT;
    
    TextBuffer textBuffer = {0};
    
    //reading a file and breaking into lines
    {
        //char *file_name = "test_file.c";
        char *fileName = "../STB/stb_truetype.h";
        //char *file_name = "checker.ts";
        //char *file_name = "SDL2.dll";
        //char *file_name = "testfile_20MB";
        
        //file name as cmd line arg
        if(argc > 1)
        {
            fileName = argv[1];
        }
        
        File file = ReadFileFromDisk(fileName);
        
        if(file.loaded)
        {
            u32 nLines = GetLineCount(file.buffer, file.size);
            
            textBuffer.capacity = nLines * 2;
            textBuffer.lines = (TextSequence*)malloc(sizeof(TextSequence) * textBuffer.capacity);
            textBuffer.lowestLine = 0;
            textBuffer.currentLine = 0;
            textBuffer.maxLinesVisible = 1 + (SCREEN_HEIGHT / fontData->lineHeight);
            textBuffer.preSize = 0;
            textBuffer.postSize = nLines - 1;
            textBuffer.gapSize = textBuffer.capacity - nLines;
            
            BreakFileIntoLines(file.buffer, file.size, nLines, &textBuffer);
        }
        
        free(file.buffer);
    }
    
    b32 quit = false;
    
    u32 gotoLine = 0;
    
    u64 frames = 0;
    u64 prevTime = 0;
    
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
                    if(textBuffer.lines[textBuffer.currentLine].preSize > 0)
                    {
                        DeleteItem(&textBuffer.lines[textBuffer.currentLine]);
                    }
                    else
                    {
                        DeleteLine(&textBuffer);
                    }
                }
                else if(event.key.keysym.sym == SDLK_ESCAPE)
                {
                    quit = true;
                }
                else if(event.key.keysym.sym == SDLK_RETURN)
                {
                    InsertLine(&textBuffer);
                }
                else if(event.key.keysym.sym == SDLK_TAB)
                {
                    InsertItem(&textBuffer.lines[textBuffer.currentLine], '\t');
                }
                else if(event.key.keysym.sym == SDLK_LEFT)
                {
                    if(textBuffer.lines[textBuffer.currentLine].preSize > 0)
                    {
                        MoveCursorLeft(&textBuffer.lines[textBuffer.currentLine]);
                    }
                    else
                    {
                        MoveCursorUpGlobal(&textBuffer);
                        
                        while(textBuffer.lines[textBuffer.currentLine].postSize > 0)
                        {
                            MoveCursorRight(&textBuffer.lines[textBuffer.currentLine]);
                        }
                    }
                }
                else if(event.key.keysym.sym == SDLK_RIGHT)
                {
                    if(textBuffer.lines[textBuffer.currentLine].postSize > 0)
                    {
                        MoveCursorRight(&textBuffer.lines[textBuffer.currentLine]);
                    }
                    else
                    {
                        MoveCursorDownGlobal(&textBuffer);
                        
                        while(textBuffer.lines[textBuffer.currentLine].preSize > 0)
                        {
                            MoveCursorLeft(&textBuffer.lines[textBuffer.currentLine]);
                        }
                    }
                }
                else if(event.key.keysym.sym == SDLK_UP)
                {
                    MoveCursorUpGlobal(&textBuffer);
                }
                else if(event.key.keysym.sym == SDLK_DOWN)
                {
                    MoveCursorDownGlobal(&textBuffer);
                }
                else if(leftCtrlDown)
                {
                    for(u32 n = 0; n < 100; n++)
                    {
                        MoveCursorDownGlobal(&textBuffer);
                    }
                    textBuffer.lowestLine = textBuffer.currentLine - (textBuffer.maxLinesVisible / 2);
                }
                else if(rightCtrlDown)
                {
                    for(u32 n = 0; n < 100; n++)
                    {
                        MoveCursorUpGlobal(&textBuffer);
                    }
                    textBuffer.lowestLine = textBuffer.currentLine - (textBuffer.maxLinesVisible / 2);
                }
                break;
                
                case SDL_TEXTINPUT:
                InsertItem(&textBuffer.lines[textBuffer.currentLine], event.text.text[0]);
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
                    
                    textBuffer.maxLinesVisible = (h / caret.height) + 1;
                }
                break;
            }
        }
        
        //scrolling by changing lowest visible line number
        if(textBuffer.currentLine >= (textBuffer.lowestLine + textBuffer.maxLinesVisible - 1))
        {
            textBuffer.lowestLine++;
        }
        else if(textBuffer.currentLine < textBuffer.lowestLine)
        {
            textBuffer.lowestLine--;
        }
        
        u32 highestLineNumber = (textBuffer.capacity - textBuffer.postSize) + (textBuffer.maxLinesVisible - (textBuffer.currentLine - textBuffer.lowestLine));
        highestLineNumber = highestLineNumber < (textBuffer.capacity - 1) ? highestLineNumber : (textBuffer.capacity - 1);
        
        //printf("pre: %d, post: %d, cl: %d, ll: %d, hl: %d, max line: %d, gap: %d\n", textBuffer.preSize, textBuffer.postSize, textBuffer.currentLine, textBuffer.lowestLine, highestLineNumber, textBuffer.maxLinesVisible, textBuffer.gapSize);
        
        u32 nDigits = 0;
        u32 num = (textBuffer.lowestLine + textBuffer.maxLinesVisible);
        
        while(num)
        {
            num = num / 10;
            nDigits++;
        }
        
        lineMargin.width = 5 + nDigits * fontData->charDatas[(u32)('0')].xadvance;
        
        //caret position
        {
            caret.x = lineMargin.width;
            for(u32 n = 0; n < textBuffer.lines[textBuffer.currentLine].preSize; n++)
            {
                u8 c = textBuffer.lines[textBuffer.currentLine].buffer[n];
                
                if(c == '\t') //tab
                {
                    caret.x += 4 * fontData->charDatas[(u32)(' ')].xadvance;
                }
                else
                {
                    caret.x += fontData->charDatas[(u32)c].xadvance;
                }
                caret.width = fontData->charDatas[(u32)c].xadvance;
            }
        }
        
        caret.y = (textBuffer.currentLine - textBuffer.lowestLine) * caret.height;
        lineHighlight.y = caret.y;
        
        //Rendering
        {
            ClearBuffer(&displayBuffer, (Color){9, 13, 18, 255});
            
            //line margin
            DrawRect(&displayBuffer, &lineMargin, (Color){26, 38, 52, 255});
            
            //line highlight
            DrawRect(&displayBuffer, &lineHighlight, (Color){26, 38, 52, 255});
            
            //caret
            DrawRect(&displayBuffer, &caret, (Color){238, 232, 0, 255});
            
            //line numbers
            {
                u32 y = 0;
                for(u32 n  = textBuffer.lowestLine; n < (textBuffer.lowestLine + textBuffer.maxLinesVisible); n++)
                {
                    u32 nDigits = 0;
                    u32 num = n + 1;
                    
                    while(num)
                    {
                        num = num / 10;
                        nDigits++;
                    }
                    
                    u8 *lineNumText = (u8*)malloc(nDigits);
                    sprintf(lineNumText, "%d", n + 1);
                    RenderText(&displayBuffer, lineNumText, nDigits, fontData, 0, y, (Color){255, 255, 255, 255});
                    free(lineNumText);
                    y += fontData->lineHeight;
                }
            }
            
            //lowestLineNumber to preSize
            u32 y = 0;
            for(u32 n = textBuffer.lowestLine; n < textBuffer.preSize; n++)
            {
                TextSequence *tSeq = &textBuffer.lines[n];
                
                u32 size = tSeq->preSize + tSeq->postSize;
                u8* text = (u8*)malloc(size);
                
                memset(tSeq->colorIndexBuffer, 3, size);
                memcpy(text, tSeq->buffer, tSeq->preSize);
                
                if(tSeq->postSize > 0)
                {
                    memcpy(text + tSeq->preSize, tSeq->buffer + (tSeq->bufferCapacity - tSeq->postSize), tSeq->postSize);
                }
                
                Lexer(text, size, tSeq->colorIndexBuffer, size);
                
                b32 activeLine = (n == textBuffer.currentLine) ? true : false;
                RenderTextSequence(&displayBuffer, tSeq, fontData, lineMargin.width, y, activeLine);
                y += fontData->lineHeight;
                
                free(text);
            }
            
            //postIndex to highestLineNumber
            if(textBuffer.postSize > 0)
            {
                for(u32 n = (textBuffer.capacity - textBuffer.postSize); n <= highestLineNumber; n++)
                {
                    TextSequence *tSeq = &textBuffer.lines[n];
                    
                    u32 size = tSeq->preSize + tSeq->postSize;
                    u8* text = (u8*)malloc(size);
                    
                    memset(tSeq->colorIndexBuffer, 3, size);
                    memcpy(text, tSeq->buffer, tSeq->preSize);
                    
                    if(tSeq->postSize > 0)
                    {
                        memcpy(text+ tSeq->preSize, tSeq->buffer + (tSeq->bufferCapacity - tSeq->postSize), tSeq->postSize);
                    }
                    
                    Lexer(text, size, tSeq->colorIndexBuffer, size);
                    
                    b32 activeLine = (n == textBuffer.currentLine) ? true : false;
                    RenderTextSequence(&displayBuffer, tSeq, fontData, lineMargin.width, y, activeLine);
                    y += fontData->lineHeight;
                    
                    free(text);
                }
            }
            
            //fps meter
            u8 fpsText[10] = {0};
            sprintf(fpsText, "%0.0f fps",  frames * 1000.0f / (SDL_GetTicks()));
            RenderText(&displayBuffer, fpsText, 10, fontData, displayBuffer.width - 100, 0, (Color){255, 255, 255, 255});
            
            SDL_UpdateTexture(texture, NULL, displayBuffer.data, 4 * displayBuffer.width);
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);
            
            frames++;
        }
    }
    
    SDL_Quit();
    return 0;
}