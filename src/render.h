#ifndef RENDER_H
#define RENDER_H

#include "types.h"

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

#endif //RENDER_H