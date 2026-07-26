#include "sed_editor.h"

// EVENT HANDLING FUNCTIONS
void FileListEvent(SDL_Event *event, Editor *editor)
{
    switch (event->type)
    {
    case SDL_KEYDOWN:
        if (event->key.keysym.sym == SDLK_ESCAPE)
        {
        }
        else if (event->key.keysym.sym == SDLK_BACKSPACE)
        {
            GoUpDirectory(&editor->fileList);
            ListFileInDirectory(&editor->fileList);
        }
        else if (event->key.keysym.sym == SDLK_RETURN)
        {
            u32 index = editor->fileList.selectedIndex;
            if (editor->fileList.files[index].dwFileAttributes != FILE_ATTRIBUTE_ARCHIVE)
            {
                u32 size = strlen(editor->fileList.currentDir);
                editor->fileList.currentDir[size] = '\\';
                memcpy(editor->fileList.currentDir + size + 1, editor->fileList.files[index].cFileName,
                       strlen(editor->fileList.files[index].cFileName));

                u32 i = 1 + size + strlen(editor->fileList.files[index].cFileName);
                editor->fileList.currentDir[i] = 0;

                printf("dir path: %s\n", editor->fileList.currentDir);
                ListFileInDirectory(&editor->fileList);
            }
            else
            {
                u32 sizeDirPath = strlen(editor->fileList.currentDir);
                u32 sizeFileName = strlen(editor->fileList.files[index].cFileName);

                u8 *path = (u8 *)malloc(sizeFileName + sizeDirPath + 3);

                memcpy(path, editor->fileList.currentDir, sizeDirPath);
                path[sizeDirPath] = '\\';
                memcpy(path + sizeDirPath + 1, editor->fileList.files[index].cFileName, sizeFileName);
                path[sizeFileName + sizeDirPath + 1] = 0;
                printf("file path: %s\n", path);

                u8 *buffer = ReadFileIntoMemory(path);

                if (buffer)
                {
                    printf("File size: %zd\n", strlen(buffer));                    
                }

                free(path);
            }
        }
        else if (event->key.keysym.sym == SDLK_UP)
        {
            if (editor->fileList.selectedIndex > 2)
            {
                editor->fileList.selectedIndex--;
            }
            else
            {
                editor->fileList.selectedIndex = editor->fileList.fileCount - 1;
            }

            printf("file: %s, type: %I32u\n", editor->fileList.files[editor->fileList.selectedIndex].cFileName,
                   editor->fileList.files[editor->fileList.selectedIndex].dwFileAttributes);
        }
        else if (event->key.keysym.sym == SDLK_DOWN)
        {
            if (editor->fileList.selectedIndex < editor->fileList.fileCount - 1)
            {
                editor->fileList.selectedIndex++;
            }
            else
            {
                editor->fileList.selectedIndex = 2;
            }

            printf("file: %s, type: %I32u\n", editor->fileList.files[editor->fileList.selectedIndex].cFileName,
                   editor->fileList.files[editor->fileList.selectedIndex].dwFileAttributes);
        }
        break;
    }
}

void GotoLineEvent(SDL_Event *event, Editor *editor)
{
    switch (event->type)
    {
    case SDL_KEYDOWN:
        if (event->key.keysym.sym == SDLK_BACKSPACE)
        {
        }
        else if (event->key.keysym.sym == SDLK_RETURN)
        {
            editor->mode = EDITOR_MODE_EDIT;
        }
        break;

    case SDL_TEXTINPUT:
        break;
    }
}

void EntryEvent(SDL_Event *event, Editor *editor)
{
    switch (event->type)
    {
    case SDL_KEYDOWN:

        b32 leftCtrlDown = (SDL_GetModState() & KMOD_LCTRL) == KMOD_LCTRL;
        b32 rightCtrlDown = (SDL_GetModState() & KMOD_RCTRL) == KMOD_RCTRL;
        b32 ctrlDown = (leftCtrlDown || rightCtrlDown);

        if (ctrlDown && event->key.keysym.sym == SDLK_o)
        {
            editor->mode = EDITOR_MODE_FILE_LIST;
            ListFileInDirectory(&editor->fileList);
        }
        break;
    }
}

void EditEvent(SDL_Event *event, Editor *editor)
{

    switch (event->type)
    {
    case SDL_KEYDOWN:
        b32 leftCtrlDown = (SDL_GetModState() & KMOD_LCTRL) == KMOD_LCTRL;
        b32 rightCtrlDown = (SDL_GetModState() & KMOD_RCTRL) == KMOD_RCTRL;
        b32 ctrlDown = (leftCtrlDown || rightCtrlDown);

        if (event->key.keysym.sym == SDLK_BACKSPACE)
        {
        }
        else if (event->key.keysym.sym == SDLK_RETURN)
        {
        }
        else if (event->key.keysym.sym == SDLK_TAB)
        {
        }
        else if (event->key.keysym.sym == SDLK_LEFT)
        {
        }
        else if (event->key.keysym.sym == SDLK_RIGHT)
        {
        }
        else if (event->key.keysym.sym == SDLK_UP)
        {
        }
        else if (event->key.keysym.sym == SDLK_DOWN)
        {
        }
        else if (ctrlDown && event->key.keysym.sym == SDLK_d)
        {
        }
        else if (ctrlDown && event->key.keysym.sym == SDLK_g)
        {
            editor->mode = EDITOR_MODE_GOTO_LINE;
        }
        else if (ctrlDown && event->key.keysym.sym == SDLK_o)
        {
            // open file
            editor->mode = EDITOR_MODE_FILE_LIST;
            ListFileInDirectory(&editor->fileList);
        }
        else if (ctrlDown && event->key.keysym.sym == SDLK_s) // save file
        {
        }
        break;

    case SDL_MOUSEWHEEL:
        if (event->wheel.y > 0) // scroll up
        {
        }
        else if (event->wheel.y < 0) // scroll down
        {
        }
        break;

    case SDL_TEXTINPUT:
        break;
    }
}

void EditorSpaceEvent(SDL_Event *event, Editor *editor)
{
    switch (editor->mode)
    {
    case EDITOR_MODE_EDIT:
        EditEvent(event, editor);
        break;

    case EDITOR_MODE_GOTO_LINE:
        GotoLineEvent(event, editor);
        break;

    case EDITOR_MODE_FILE_LIST:
        FileListEvent(event, editor);
        break;

    case EDITOR_MODE_ENTRY:
        EntryEvent(event, editor);
        break;
    }
}

// RENDERING FUNCTIONS

void EditorUpdateAndRender(Buffer *renderBuffer, FontData *fontData, Editor *editor)
{

    Rect header = {0};
    header.x = editor->rect.x;
    header.y = editor->rect.y;
    header.width = editor->rect.width;
    header.height = fontData->lineHeight + 4;

    Rect lineMargin = {0};
    lineMargin.x = header.x;
    lineMargin.y = header.y + header.height;
    lineMargin.width = 10 + 100 * fontData->charDatas[(u32)('0')].xadvance;
    lineMargin.height = editor->rect.height - header.height;

    Rect caret = {0};
    caret.x = lineMargin.x + lineMargin.width;
    caret.y = lineMargin.y;
    caret.width = fontData->charDatas[(u32)('A')].xadvance;
    caret.height = fontData->lineHeight;

    Rect lineHighlight = {0};
    lineHighlight.x = caret.x;
    lineHighlight.y = caret.y;
    lineHighlight.width = editor->rect.width - lineMargin.width;
    lineHighlight.height = caret.height;

    // caret(cursor) position
    {
    }

    // scrolling by changing lowest visible line number
    {
    }

    // Rendering
    {
        // header
        DrawRect(renderBuffer, &header, (Color){214, 181, 139, 255});

        // line margin
        DrawRect(renderBuffer, &lineMargin, (Color){4, 35, 40, 255});

        // line highlight
        DrawRect(renderBuffer, &lineHighlight, (Color){28, 60, 55, 255});

        // caret(cursor)
        DrawRect(renderBuffer, &caret, (Color){135, 223, 148, 255});

        // goto line
        if (editor->mode == EDITOR_MODE_GOTO_LINE)
        {
        }
    }

    // editor space border
    DrawRectWire(renderBuffer, &editor->rect, (Color){255, 255, 255, 255});
}

void RenderFileLister(Buffer *renderBuffer, FontData *fontData, Editor *editor)
{
    FileList *fileList = &editor->fileList;
    Color textColor = {204, 190, 164, 255};

    // current directory header
    Rect headerRect = {0};
    headerRect.x = editor->rect.x;
    headerRect.y = editor->rect.y;
    headerRect.width = editor->rect.width;
    headerRect.height = 1.5f * fontData->lineHeight;

    // DrawRectWire(renderBuffer, &headerRect, (Color){214, 181, 139, 255});

    u32 endX = RenderText(renderBuffer, "Open:\0", strlen("Open:\0"), fontData, headerRect.x + 5,
                          headerRect.y + (headerRect.height - fontData->lineHeight) / 2, (Color){255, 255, 0, 255},
                          editor->rect);

    RenderText(renderBuffer, fileList->currentDir, strlen(fileList->currentDir), fontData, endX + 10,
               headerRect.y + (headerRect.height - fontData->lineHeight) / 2, textColor, editor->rect);

    Rect rect = {0};
    rect.x = editor->rect.x;
    rect.y = editor->rect.y + headerRect.height;
    rect.width = editor->rect.width;
    rect.height = 2 * fontData->lineHeight;

    for (u32 n = 2; n < fileList->fileCount; n++)
    {
        if (n == fileList->selectedIndex)
        {
            DrawRect(renderBuffer, &rect, (Color){8, 70, 80, 255});
            textColor = (Color){255, 255, 0, 255};
        }
        else
        {
            textColor = (Color){204, 190, 164, 255};
        }

        if (fileList->files[n].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            // directory

            // icon rendering
            u32 end =
                RenderText(renderBuffer, "d\0", 1, fileList->fileIconFont, rect.x + 10,
                           rect.y + (rect.height - fileList->fileIconFont->lineHeight) / 2, textColor, editor->rect);

            end = RenderText(renderBuffer, fileList->files[n].cFileName, strlen(fileList->files[n].cFileName), fontData,
                             end + 15, rect.y + (fontData->lineHeight / 2), textColor, editor->rect);
            // RenderText(renderBuffer, "\\\0", 1, fontData, end, rect.y + (fontData->lineHeight / 2), textColor,
            // editor->rect);
        }
        else
        {
            // file

            // icon rendering
            u32 end =
                RenderText(renderBuffer, "f\0", 1, fileList->fileIconFont, rect.x + 10,
                           rect.y + (rect.height - fileList->fileIconFont->lineHeight) / 2, textColor, editor->rect);

            RenderText(renderBuffer, fileList->files[n].cFileName, strlen(fileList->files[n].cFileName), fontData,
                       end + 15, rect.y + (fontData->lineHeight / 2), textColor, editor->rect);
        }
        rect.y += rect.height;
    }
}

void RenderEntry(Buffer *renderBuffer, FontData *fontData, Editor *editor)
{
    u8 *welcomeText = "Welcome to SmallEd!\0";

    u32 width = strlen(welcomeText) * (fontData->charDatas['A'].x1 - fontData->charDatas['A'].x0);
    u32 x = (editor->rect.width / 2) - (width / 2);
    u32 y = editor->rect.y + editor->rect.height / 2;

    RenderText(renderBuffer, welcomeText, strlen(welcomeText), fontData, x, y, (Color){255, 255, 255, 255},
               editor->rect);

    u8 *openText = "Open File - Ctrl + O\0";

    width = strlen(openText) * (fontData->charDatas['A'].x1 - fontData->charDatas['A'].x0);
    x = (editor->rect.width / 2) - (width / 2);
    y = (editor->rect.height / 2) + fontData->lineHeight + 4;

    u32 end = RenderText(renderBuffer, openText, strlen("Open File - \0"), fontData, x, y, (Color){255, 255, 255, 255},
                         editor->rect);
    RenderText(renderBuffer, "Ctrl + O\0", strlen("Ctrl + O\0"), fontData, end, y, (Color){255, 255, 0, 255},
               editor->rect);
}

void AppUpdateAndRender(Buffer *renderBuffer, FontData *fontData, Editor *editor)
{
    switch (editor->mode)
    {
    case EDITOR_MODE_EDIT:
    case EDITOR_MODE_GOTO_LINE:
        EditorUpdateAndRender(renderBuffer, fontData, editor);
        break;

    case EDITOR_MODE_FILE_LIST:
        RenderFileLister(renderBuffer, fontData, editor);
        break;

    case EDITOR_MODE_ENTRY:
        RenderEntry(renderBuffer, fontData, editor);
        break;
    }
}
