#ifndef UI_H
#define UI_H

#include "types.h"
#include "render.h"

#define MAX_LIST_CAPACITY 100

//for directory and file listing
typedef struct ListMenu
{
    u32 length;
    u8 *itemName[MAX_LIST_CAPACITY];
    u32 selectedItem;
}LisrtMenu;

void RenderListMenu(Buffer renderBuffer, ListMenu *listMenu)
{
    return;
}

#endif //UI_H
