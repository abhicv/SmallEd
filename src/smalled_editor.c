#include "smalled_editor.h"

u32 DigitCount(u32 n)
{
    u32 count = 0;

    while(n)
    {
        n = n / 10;
        count++;
    }

    return count;
}

u32 Power(u32 x, u32 power)
{
    u32 result = 1;

    for(u32 n = 0; n < power; n++)
    {
        result *= x;
    }

    return result;
}

u32 StrToNum(const u8 *str, u32 size)
{
    u32 num = 0;

    u32 power = size - 1;

    for(u32 n = 0; n < size; n++)
    {
        num += (str[n] - '0') * Power(10, power);
        power--;
    }

    return num;
}

void FileListEvent(SDL_Event *event, Editor *editor)
{
    switch(event->type)
    {
        case SDL_KEYDOWN:
        if(event->key.keysym.sym == SDLK_ESCAPE)
        {
            editor->mode = EDITOR_MODE_EDIT;
        }
        else if(event->key.keysym.sym == SDLK_RETURN)
        {
            u32 index = editor->fileList.selectedIndex;

            char *fileName = editor->fileList.fileDatas[index].cFileName;

            File file = ReadFileFromDisk(fileName);

            if(file.loaded)
            {
                u32 nLines = GetLineCount(file.buffer, file.size);

                printf("line count: %d\n", nLines);

                TextBuffer textBuffer = {0};

                textBuffer.capacity = 2 * nLines;
                textBuffer.lines = (TextSequence*)malloc(sizeof(TextSequence) * textBuffer.capacity);
                textBuffer.lowestLine = 0;
                textBuffer.currentLine = 0;
                textBuffer.preSize = 0;
                textBuffer.postSize = nLines - 1;
                textBuffer.gapSize = textBuffer.capacity - nLines;

                BreakFileIntoLines(file.buffer, file.size, nLines, &textBuffer);
                editor->textBuffer = textBuffer;

                editor->fileName = fileName;
                editor->mode = EDITOR_MODE_EDIT;
            }

            free(file.buffer);
            file.buffer = NULL;
        }
        else if(event->key.keysym.sym == SDLK_UP)
        {
            if(editor->fileList.selectedIndex > 2)
            {
                editor->fileList.selectedIndex--;
            }
            else
            {
                editor->fileList.selectedIndex = editor->fileList.fileCount - 1;
            }
        }
        else if(event->key.keysym.sym == SDLK_DOWN)
        {
            if(editor->fileList.selectedIndex < editor->fileList.fileCount - 1)
            {
                editor->fileList.selectedIndex++;
            }
            else
            {
                editor->fileList.selectedIndex = 2;
            }
        }

        break;
    }
}

void GotoLineEvent(SDL_Event *event, Editor *editor)
{
    TextBuffer *textBuffer = &editor->textBuffer;

    switch(event->type)
    {
        case SDL_KEYDOWN:

        if(event->key.keysym.sym == SDLK_BACKSPACE)
        {
            DeleteItem(&editor->gotoLineTSeq);
        }
        else if(event->key.keysym.sym == SDLK_RETURN)
        {
            if(editor->gotoLineTSeq.preSize > 0)
            {
                u32 lineNum = StrToNum(editor->gotoLineTSeq.buffer, editor->gotoLineTSeq.preSize);
                GotoLine(textBuffer, lineNum);

                editor->gotoLineTSeq.gapSize = editor->gotoLineTSeq.bufferCapacity;
                editor->gotoLineTSeq.preSize = 0;
                editor->gotoLineTSeq.postSize = 0;
            }

            editor->mode = EDITOR_MODE_EDIT;
        }
        break;

        case SDL_TEXTINPUT:
        u8 n = event->text.text[0];

        if(n >= '0' && n <= '9')
        {
            InsertItem(&editor->gotoLineTSeq, n);
        }
        break;
    }
}

void EntryEvent(SDL_Event *event, Editor *editor)
{
    switch(event->type)
    {
        case SDL_KEYDOWN:
        b32 leftCtrlDown = (SDL_GetModState() & KMOD_LCTRL) == KMOD_LCTRL;
        b32 rightCtrlDown = (SDL_GetModState() & KMOD_RCTRL) == KMOD_RCTRL;
        b32 ctrlDown = (leftCtrlDown || rightCtrlDown);

        if(ctrlDown && event->key.keysym.sym == SDLK_o)
        {
            editor->mode = EDITOR_MODE_FILE_LIST;
            ListFileInDirectory(&editor->fileList);
        }
        break;
    }
}

void EditEvent(SDL_Event *event, Editor *editor)
{
    TextBuffer *textBuffer = &editor->textBuffer;
    TextSequence *tSeq = &editor->textBuffer.lines[textBuffer->currentLine];

    switch(event->type)
    {
        case SDL_KEYDOWN:
        b32 leftCtrlDown = (SDL_GetModState() & KMOD_LCTRL) == KMOD_LCTRL;
        b32 rightCtrlDown = (SDL_GetModState() & KMOD_RCTRL) == KMOD_RCTRL;
        b32 ctrlDown = (leftCtrlDown || rightCtrlDown);

        if(event->key.keysym.sym == SDLK_BACKSPACE)
        {
            if(tSeq->preSize > 0)
            {
                DeleteItem(tSeq);
            }
            else
            {
                DeleteLine(textBuffer);
            }
        }
        else if(event->key.keysym.sym == SDLK_RETURN)
        {
            InsertLine(textBuffer);
        }
        else if(event->key.keysym.sym == SDLK_TAB)
        {
            InsertItem(tSeq, '\t');
        }
        else if(event->key.keysym.sym == SDLK_LEFT)
        {
            MoveCursorLeft(textBuffer);
        }
        else if(event->key.keysym.sym == SDLK_RIGHT)
        {
            MoveCursorRight(textBuffer);
        }
        else if(event->key.keysym.sym == SDLK_UP)
        {
            MoveCursorUp(textBuffer);
        }
        else if(event->key.keysym.sym == SDLK_DOWN)
        {
            MoveCursorDown(textBuffer);
        }
        else if(ctrlDown && event->key.keysym.sym == SDLK_g)
        {
            editor->mode = EDITOR_MODE_GOTO_LINE;
        }
        else if(ctrlDown && event->key.keysym.sym == SDLK_o)
        {
            //open file
            editor->mode = EDITOR_MODE_FILE_LIST;
            ListFileInDirectory(&editor->fileList);
        }
        break;

        case SDL_TEXTINPUT:
        InsertItem(tSeq, event->text.text[0]);

        if(event->text.text[0] == '"')
        {
            InsertItem(tSeq, '"');
            MoveCursorLeftLocal(tSeq);
        }
        else if(event->text.text[0] == '{')
        {
            InsertItem(tSeq, '}');
            MoveCursorLeftLocal(tSeq);
        }
        else if(event->text.text[0] == '(')
        {
            InsertItem(tSeq, ')');
            MoveCursorLeftLocal(tSeq);
        }
        else if(event->text.text[0] == '<')
        {
            InsertItem(tSeq, '>');
            MoveCursorLeftLocal(tSeq);
        }

        break;
    }
}

void EditorSpaceEvent(SDL_Event *event, Editor *editor)
{
    switch(editor->mode)
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

void RenderEditorSpace(Buffer *renderBuffer, FontData *fontData, Editor *editor)
{
    TextBuffer *textBuffer = &editor->textBuffer;

    Rect header = {0};
    header.x = editor->rect.x;
    header.y = editor->rect.y;
    header.width = editor->rect.width;
    header.height = fontData->lineHeight + 4;

    Rect lineMargin = {0};
    lineMargin.x = header.x;
    lineMargin.y = header.y + header.height;
    lineMargin.width = 8 + DigitCount(textBuffer->preSize + textBuffer->postSize) * fontData->charDatas[(u32)('0')].xadvance;
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

    //caret(cursor) position
    {
        u8 *lineBuffer = textBuffer->lines[textBuffer->currentLine].buffer;

        for(u32 n = 0; n < textBuffer->lines[textBuffer->currentLine].preSize; n++)
        {
            u8 c = lineBuffer[n];

            if(c == '\t') //tab
            {
                caret.x += 4 * fontData->charDatas[(u32)(' ')].xadvance;
            }
            else if(RENDERABLE_CHAR(c))
            {
                caret.x += fontData->charDatas[(u32)c].xadvance;
            }
            else
            {
                caret.x += fontData->charDatas[(u32)('A')].xadvance;
            }

            caret.width = fontData->charDatas[(u32)c].xadvance;
        }
    }

    textBuffer->maxLinesVisible = ((editor->rect.height - header.height) / fontData->lineHeight) + 1;

    //scrolling by changing lowest visible line number
    {
        if(textBuffer->currentLine >= (textBuffer->lowestLine + textBuffer->maxLinesVisible - 1))
        {
            textBuffer->lowestLine++;
        }
        else if(textBuffer->currentLine < textBuffer->lowestLine)
        {
            textBuffer->lowestLine--;
        }
    }

    u32 highestLineNumber = (textBuffer->capacity - textBuffer->postSize) + (textBuffer->maxLinesVisible - (textBuffer->currentLine - textBuffer->lowestLine));

    highestLineNumber = highestLineNumber < (textBuffer->capacity - 1) ? highestLineNumber : (textBuffer->capacity - 1);

    caret.y = caret.y + (textBuffer->currentLine - textBuffer->lowestLine) * caret.height;
    lineHighlight.y = caret.y;

    //Rendering
    {
        //header
        DrawRect(renderBuffer, &header, (Color){214, 181, 139, 255});

        //line margin
        DrawRect(renderBuffer, &lineMargin, (Color){4, 35, 40, 255});

        //line highlight
        DrawRect(renderBuffer, &lineHighlight, (Color){28, 60, 55, 255});

        //caret
        DrawRect(renderBuffer, &caret, (Color){135, 223, 148, 255});

        u32 totalLineCount = textBuffer->preSize + textBuffer->postSize;

        f32 percent = (f32)(textBuffer->currentLine + 1) * 100.0f / (f32)totalLineCount;

        u8 headerText[250] = {0};
        sprintf(headerText, "%s - line: %d\tCol: %d\t%0.1f %%\0", editor->fileName, textBuffer->currentLine + 1, textBuffer->lines[textBuffer->currentLine].preSize + 1, percent);

        RenderText(renderBuffer, headerText, strlen(headerText), fontData, header.x + 4, header.y + 2, (Color){10, 10, 10, 255});

        //line numbers
        {
            u32 y = lineMargin.y;
            for(u32 n  = textBuffer->lowestLine; n <= (textBuffer->lowestLine + textBuffer->maxLinesVisible); n++)
            {
                u32 nDigits = DigitCount(n + 1);

                u8 *lineNumText = (u8*)malloc(nDigits + 1);
                lineNumText[nDigits] = 0;
                sprintf(lineNumText, "%d", n + 1);

                u32 x = nDigits * fontData->charDatas[(u32)'0'].xadvance + 4;

                if(n == textBuffer->currentLine)
                {
                    RenderText(renderBuffer, lineNumText, nDigits, fontData, lineMargin.width - x, y, (Color){255, 255, 255, 255});
                }
                else
                {
                    RenderText(renderBuffer, lineNumText, nDigits, fontData, lineMargin.width - x, y, (Color){100, 100, 100, 255});
                }

                free(lineNumText);

                y += fontData->lineHeight;
            }
        }

        //NOTE(abhicv): lowestLineNumber to preSize
        u32 y = lineMargin.y;
        b32 multiCommentLine = false;

        for(u32 n = textBuffer->lowestLine; n < textBuffer->preSize; n++)
        {
            TextSequence *tSeq = &textBuffer->lines[n];

            u32 size = tSeq->preSize + tSeq->postSize;
            u8* text = (u8*)malloc(size);

            memset(tSeq->colorIndexBuffer, 3, size);
            memcpy(text, tSeq->buffer, tSeq->preSize);

            if(tSeq->postSize > 0)
            {
                memcpy(text + tSeq->preSize, tSeq->buffer + (tSeq->bufferCapacity - tSeq->postSize), tSeq->postSize);
            }

            Lexer(text, size, tSeq->colorIndexBuffer, size, &multiCommentLine);

            b32 activeLine = (n == textBuffer->currentLine) ? true : false;
            RenderTextSequence(renderBuffer, tSeq, fontData, lineMargin.width, y, activeLine);
            y += fontData->lineHeight;

            free(text);
        }

        //NOTE(abhicv): postIndex to highestLineNumber
        if(textBuffer->postSize > 0)
        {
            for(u32 n = (textBuffer->capacity - textBuffer->postSize); n <= highestLineNumber; n++)
            {
                TextSequence *tSeq = &textBuffer->lines[n];

                u32 size = tSeq->preSize + tSeq->postSize;
                u8* text = (u8*)malloc(size);

                memset(tSeq->colorIndexBuffer, 3, size);
                memcpy(text, tSeq->buffer, tSeq->preSize);

                if(tSeq->postSize > 0)
                {
                    memcpy(text+ tSeq->preSize, tSeq->buffer + (tSeq->bufferCapacity - tSeq->postSize), tSeq->postSize);
                }

                Lexer(text, size, tSeq->colorIndexBuffer, size, &multiCommentLine);

                RenderTextSequence(renderBuffer, tSeq, fontData, lineMargin.width, y, false);
                y += fontData->lineHeight;

                free(text);
            }
        }

        if(editor->mode == EDITOR_MODE_GOTO_LINE)
        {
            Rect gotoRect = {0};
            gotoRect.x = 0;
            gotoRect.y = header.y + header.height;
            gotoRect.width = header.width;
            gotoRect.height = header.height;
            DrawRect(renderBuffer, &gotoRect, (Color){4, 35, 40, 255});

            u8 *s = "Goto Line :\0";

            u32 end = RenderText(renderBuffer, s, strlen(s), fontData, gotoRect.x + 4, gotoRect.y + 2, (Color){255, 255, 255, 255});
            RenderTextSequenceSimple(renderBuffer, &editor->gotoLineTSeq, fontData, end + 5, gotoRect.y + 2, (Color){200, 200, 200, 255});
        }
    }
}

void RenderFileLister(Buffer *renderBuffer, FontData *fontData, Editor *editor)
{
    Rect rect = {0};
    rect.x = 0;
    rect.y = 0;
    rect.width = editor->rect.width;
    rect.height = 2 * fontData->lineHeight;

    FileList *fileList = &editor->fileList;

    Color textColor = {204, 190, 164, 255};

    for(u32 n = 2; n < fileList->fileCount; n++)
    {
        if(n == fileList->selectedIndex)
        {
            DrawRect(renderBuffer, &rect, (Color){214, 181, 139, 255});
            textColor = (Color){0, 0, 0, 255};
        }
        else
        {
            DrawRectWire(renderBuffer, &rect, (Color){135, 223, 148, 255});
            textColor = (Color){204, 190, 164, 255};
        }

        if(fileList->fileDatas[n].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            // directory
            u32 end = RenderText(renderBuffer, fileList->fileDatas[n].cFileName, strlen(fileList->fileDatas[n].cFileName), fontData, rect.x + 5, rect.y + (fontData->lineHeight / 2), textColor);

            RenderText(renderBuffer, "/", 1, fontData, end, rect.y + (fontData->lineHeight / 2), textColor);
        }
        else
        {
            // file
            RenderText(renderBuffer, fileList->fileDatas[n].cFileName, strlen(fileList->fileDatas[n].cFileName), fontData, rect.x + 5, rect.y + (fontData->lineHeight / 2), textColor);
        }

        rect.y += rect.height;
    }
}

void RenderEntry(Buffer *renderBuffer, FontData *fontData, Editor *editor)
{
    u8 *welcomeText = "Welcome to SmallEd!\0";

    u32 width = strlen(welcomeText) * (fontData->charDatas['A'].x1 - fontData->charDatas['A'].x0);
    u32 x = (editor->rect.width / 2) - (width / 2);
    u32 y = editor->rect.height / 2;

    RenderText(renderBuffer, welcomeText, strlen(welcomeText), fontData, x, y ,(Color){255, 255, 255, 255});

    u8 *openText = "Open File - Ctrl + O\0";

    width = strlen(openText) * (fontData->charDatas['A'].x1 - fontData->charDatas['A'].x0);
    x = (editor->rect.width / 2) - (width / 2);
    y = (editor->rect.height / 2) + fontData->lineHeight + 4;

    RenderText(renderBuffer, openText, strlen(openText), fontData, x, y ,(Color){255, 255, 255, 255});
}

void RenderSpace(Buffer *renderBuffer, FontData *fontData, Editor *editor)
{
    switch(editor->mode)
    {
        case EDITOR_MODE_EDIT:
        case EDITOR_MODE_GOTO_LINE:
        RenderEditorSpace(renderBuffer, fontData, editor);
        break;

        case EDITOR_MODE_FILE_LIST:
        RenderFileLister(renderBuffer, fontData, editor);
        break;

        case EDITOR_MODE_ENTRY:
        RenderEntry(renderBuffer, fontData, editor);
        break;
    }
}

