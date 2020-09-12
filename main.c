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
    
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, SCREEN_WIDTH, SCREEN_HEIGHT);
    
    stbtt_fontinfo font;
    FILE *fontFile = fopen("C:/Windows/Fonts/consola.ttf", "rb");
    if(fontFile != NULL)
    {
        fread(ttf_buffer, 1, 1 << 25, fontFile);
        stbtt_InitFont(&font, ttf_buffer, stbtt_GetFontOffsetForIndex(ttf_buffer, 0));
        fclose(fontFile);
    }
    
    u32 size = 23;
    f32 scale = stbtt_ScaleForPixelHeight(&font, size);
    
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&font, &ascent, &descent, &lineGap);
    ascent = roundf(ascent * scale);
    descent = roundf(descent * scale);
    
    Rect caret = {0, 0, 10, ascent + descent + 8};
    Rect lineHighlight = {0, 0, SCREEN_WIDTH, 0};
    
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
                continue;
                break;
                case SDL_KEYDOWN:
                if(event.key.keysym.sym == SDLK_BACKSPACE)
                {
                    if(cIndex > 0)
                    {
                        cIndex--;
                        textBuffer[cIndex] = 0;
                    }
                }
                else if(event.key.keysym.sym == SDLK_RETURN)
                {
                    textBuffer[cIndex] = '\n';
                    cIndex++;
                }
                else if(event.key.keysym.sym == SDLK_TAB)
                {
                    textBuffer[cIndex] = '\t';
                    cIndex++;
                }
                break;
                
                case SDL_TEXTINPUT:
                strcat(textBuffer, event.text.text);
                cIndex++;
                printf("cindex : %d\n", cIndex);
                break;
            }
        }
        
        ClearBuffer(&buffer, (Color){0, 0, 0, 0});
        
        lineHighlight.y = caret.y;
        lineHighlight.height = caret.height;
        DrawRect(&buffer, &lineHighlight, (Color){40, 40, 40, 0});
        DrawRect(&buffer, &caret, (Color){255, 255, 0, 0});
        
        //Font Rendering
        {
            u32 cursorX = 0;
            u32 baseline = ascent;
            
            u32 i = 0;
            char c = textBuffer[i];
            while(c != 0)
            {
                int advance;
                int lsb; //left side bearing
                stbtt_GetCodepointHMetrics(&font, c, &advance, &lsb);
                
                if(c == '\n')
                {
                    baseline += ascent + 4;
                    cursorX = -roundf(advance * scale);
                }
                if(c == '\t')
                {
                    cursorX += 4 * roundf(advance * scale);
                }
                if(c >= '!' && c <= '~')
                {
                    Rect charRect = {0};
                    
                    u8 *bitmap = stbtt_GetCodepointBitmap(&font, 0, scale, c, &charRect.width, &charRect.height, 0, 0);
                    
                    i32 c_x1, c_y1, c_x2, c_y2;
                    stbtt_GetCodepointBitmapBox(&font, c, scale, scale, &c_x1, &c_y1, &c_x2, &c_y2);
                    
                    charRect.x = cursorX + c_x1;
                    charRect.y = baseline + c_y1;
                    
                    RenderFontBitMap(&buffer, bitmap, &charRect);
                    free(bitmap);
                }
                
                cursorX += roundf(advance * scale);
                
                caret.x = cursorX;
                caret.y = baseline - ascent;
                caret.width = roundf(advance * scale);
                
                if(textBuffer[i + 1])
                {
                    i32 kern = stbtt_GetCodepointKernAdvance(&font, textBuffer[i], textBuffer[i + 1]);
                    cursorX += roundf(kern * scale);
                }
                
                i++;
                c = textBuffer[i];
            }
        }
        
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