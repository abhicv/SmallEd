#ifndef UI_H
#define UI_H

#include "types.h"

#define MAX_LIST_CAPACITY 100

//for directory and file listing
typedef struct ListMenu
{
    u32 length;
    u8 *itemName[MAX_LIST_CAPACITY];
    u32 selectedItem;
}

void RenderListMenu(ListMenu *listMenu)
{
    
}

#endif //UI_H
