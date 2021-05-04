#ifndef SED_RENDER_H
#define SED_RENDER_H

#include "sed_types.h"

#define HEX_TO_RGB(x) ((Color){.r = (x >> 0) & 0xFF, .g = (x >> 8) & 0xFF, .b = (x >> 16) & 0xFF, .a = 0xFF})
#define PACK_RGBA_INTO_U32(color) ((color.r << 0) | (color.g << 8) | (color.b << 16) | (color.a << 24))

typedef struct Color
{
    u8 r;
    u8 g; 
    u8 b; 
    u8 a;
    
} Color;

typedef struct Buffer
{
    u32 *data;
    u32 width;
    u32 height;
    
} Buffer;

typedef struct Rect
{
    i32 x;
    i32 y;
    u32 width;
    u32 height;
    
} Rect;

void ClearBuffer(Buffer *buffer, Color color);
void DrawRect(Buffer *buffer, Rect *rect, Color color);
void DrawRectWire(Buffer *buffer, Rect *rect, Color color);
void DrawCircle(Buffer *buffer, i32 xPos, i32 yPos, u32 radius, Color color);
void DrawRectRounded(Buffer *buffer, Rect *rect, u32 roundness, Color color);

#endif //SED_RENDER_H
