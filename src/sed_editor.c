#include "sed_editor.h"

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

u32 StringToInteger(const u8 *str, u32 size)
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
            if(editor->textBuffer.loaded)
            {
                editor->mode = EDITOR_MODE_EDIT;
            }
        }
        else if(event->key.keysym.sym == SDLK_BACKSPACE)
        {
            GoUpDirectory(&editor->fileList);
            ListFileInDirectory(&editor->fileList);
        }
        else if(event->key.keysym.sym == SDLK_RETURN)
        {
            u32 index = editor->fileList.selectedIndex;
            if(editor->fileList.files[index].dwFileAttributes != FILE_ATTRIBUTE_ARCHIVE)
            {
                u32 size = strlen(editor->fileList.currentDir);
                editor->fileList.currentDir[size] = '\\';
                memcpy(editor->fileList.currentDir + size + 1, editor->fileList.files[index].cFileName, strlen(editor->fileList.files[index].cFileName));
                
                u32 i = 1 + size + strlen(editor->fileList.files[index].cFileName);
                editor->fileList.currentDir[i] = 0;
                
                printf("dir path: %s\n", editor->fileList.currentDir);
                ListFileInDirectory(&editor->fileList);
            }
            else
            {
                u32 sizeDirPath = strlen(editor->fileList.currentDir);
                u32 sizeFileName = strlen(editor->fileList.files[index].cFileName);
                
                u8 *path = (u8*)malloc(sizeFileName + sizeDirPath + 3);
                
                memcpy(path, editor->fileList.currentDir, sizeDirPath);  
                path[sizeDirPath] = '\\';
                memcpy(path + sizeDirPath + 1, editor->fileList.files[index].cFileName, sizeFileName);
                path[sizeFileName + sizeDirPath + 1] = 0;
                printf("file path: %s\n", path);
                
                File file = ReadFileNullTerminate(path);
                
                free(path);
                
                if(file.loaded)
                {
                    u32 nLines = 0;
                    LineInfo *lineInfo = PreprocessFileBuffer(file.buffer, file.size, &nLines);
                    printf("line count: %d\n", nLines);
                    
                    //TODO(abhicv): free previous allocated textBuffer
                    
                    TextBuffer textBuffer = {0};
                    textBuffer.capacity = 2 * nLines;
                    textBuffer.lines = (TextSequence*)AllocatePersistentMemory(sizeof(TextSequence) * textBuffer.capacity);
                    textBuffer.lowestLine = 0;
                    textBuffer.currentLine = 0;
                    textBuffer.preSize = 0;
                    textBuffer.postSize = nLines - 1;
                    textBuffer.gapSize = textBuffer.capacity - nLines;
                    
                    textBuffer.lines[0].buffer = (u8*)AllocatePersistentMemory(FIXED_LINE_SIZE_IN_BYTES * textBuffer.capacity);
                    
                    for(u32 n = 1; n < textBuffer.capacity; n++)
                    {
                        textBuffer.lines[n].buffer = textBuffer.lines[0].buffer + (FIXED_LINE_SIZE_IN_BYTES * n); 
                    }
                    
                    //TODO(abhicv): reconsider colorIndexBuffer for syntax highlighting
                    textBuffer.lines[0].colorIndexBuffer = (u8*)AllocatePersistentMemory(FIXED_LINE_SIZE_IN_BYTES * textBuffer.capacity);
                    for(u32 n = 1; n < textBuffer.capacity; n++)
                    {
                        textBuffer.lines[n].colorIndexBuffer = textBuffer.lines[0].colorIndexBuffer + (FIXED_LINE_SIZE_IN_BYTES * n); 
                    }
                    
                    LoadTextBuffer(file.buffer, file.size, lineInfo, nLines, &textBuffer);
                    
                    free(lineInfo);
                    
                    editor->textBuffer = textBuffer;
                    editor->fileName = editor->fileList.files[index].cFileName;
                    
                    if(textBuffer.loaded)
                    {
                        editor->mode = EDITOR_MODE_EDIT;
                    }
                }
                
                free(file.buffer);
                file.buffer = NULL;
            }
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
            
            printf("file: %s, type: %I32u\n", editor->fileList.files[editor->fileList.selectedIndex].cFileName, editor->fileList.files[editor->fileList.selectedIndex].dwFileAttributes);
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
            printf("file: %s, type: %I32u\n", editor->fileList.files[editor->fileList.selectedIndex].cFileName, editor->fileList.files[editor->fileList.selectedIndex].dwFileAttributes);
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
                u32 lineNum = StringToInteger(editor->gotoLineTSeq.buffer, editor->gotoLineTSeq.preSize);
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
        else if(ctrlDown && event->key.keysym.sym == SDLK_d)
        {
            DeleteLine(textBuffer);
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
        else if(ctrlDown && event->key.keysym.sym == SDLK_s)
        {
            u32 size = GetTextBufferSize(&editor->textBuffer);
            u8* buffer = (u8*)malloc(size);
            memset(buffer, 0, size);
            
            TextBufferToPlainBuffer(&editor->textBuffer, buffer);
            
            u32 fileNameLen = strlen(editor->fileName) + strlen(editor->fileList.currentDir) + 2;
            u8 *fileName = (u8*)malloc(fileNameLen);
            memset(fileName, 0, fileNameLen);
            
            memcpy(fileName, editor->fileList.currentDir, strlen(editor->fileList.currentDir));
            fileName[strlen(editor->fileList.currentDir)] = '\\';
            memcpy(fileName + strlen(editor->fileList.currentDir) + 1, editor->fileName, strlen(editor->fileName));
            fileName[fileNameLen - 1] = 0;
            
            WriteFileU8(fileName, buffer, size);
            
            printf("File saved to %s\n", fileName);
            
            free(buffer);
            free(fileName);
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
    lineMargin.width = 10 + DigitCount(textBuffer->lowestLine + textBuffer->maxLinesVisible) * fontData->charDatas[(u32)('0')].xadvance;
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
    
    
    textBuffer->maxLinesVisible = ((editor->rect.height - header.height) / fontData->lineHeight) + 1;
    
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
        
        u8 headerText[512] = {0};
        sprintf(headerText, "%s - line: %d\tCol: %d\t%0.1f %%\0", editor->fileName, textBuffer->currentLine + 1, textBuffer->lines[textBuffer->currentLine].preSize + 1, percent);
        
        RenderText(renderBuffer, headerText, strlen(headerText), fontData, header.x + 4, header.y + 2, (Color){10, 10, 10, 255}, editor->rect);
        
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
                    RenderText(renderBuffer, lineNumText, nDigits, fontData, lineMargin.x + lineMargin.width - x, y, (Color){255, 255, 255, 255}, editor->rect);
                }
                else
                {
                    RenderText(renderBuffer, lineNumText, nDigits, fontData, lineMargin.x + lineMargin.width - x, y, (Color){100, 100, 100, 255}, editor->rect);
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
            
            free(text);
            
            b32 activeLine = (n == textBuffer->currentLine) ? true : false;
            RenderTextSequence(renderBuffer, tSeq, fontData, lineMargin.x + lineMargin.width, y, activeLine, editor->rect);
            y += fontData->lineHeight;
            
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
                
                free(text);
                
                RenderTextSequence(renderBuffer, tSeq, fontData, lineMargin.x + lineMargin.width, y, false, editor->rect);
                y += fontData->lineHeight;
            }
        }
        
        //goto line
        if(editor->mode == EDITOR_MODE_GOTO_LINE)
        {
            Rect gotoRect = {0};
            gotoRect.x = editor->rect.x;
            gotoRect.y = header.y + header.height;
            gotoRect.width = header.width;
            gotoRect.height = header.height;
            
            DrawRect(renderBuffer, &gotoRect, (Color){4, 35, 40, 255});
            DrawRectWire(renderBuffer, &gotoRect, (Color){214, 181, 139, 255});
            
            u8 *s = "Goto Line:\0";
            
            u32 end = RenderText(renderBuffer, s, strlen(s), fontData, gotoRect.x + 4, gotoRect.y + 2, (Color){255, 255, 255, 255}, editor->rect);
            RenderTextSequenceSimple(renderBuffer, &editor->gotoLineTSeq, fontData, end + 10, gotoRect.y + 2, (Color){200, 200, 200, 255}, editor->rect);
        }
    }
    
    //border
    DrawRectWire(renderBuffer, &editor->rect, (Color){255, 255, 255, 255});
}

void RenderFileLister(Buffer *renderBuffer, FontData *fontData, Editor *editor)
{
    Rect rect = {0};
    rect.x = editor->rect.x;
    rect.y = editor->rect.y;
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
            //DrawRectWire(renderBuffer, &rect, (Color){135, 223, 148, 255});
            textColor = (Color){204, 190, 164, 255};
        }
        
        if(fileList->files[n].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            // directory
            u32 end = RenderText(renderBuffer, fileList->files[n].cFileName, strlen(fileList->files[n].cFileName), fontData, rect.x + 5, rect.y + (fontData->lineHeight / 2), textColor, editor->rect);
            
            RenderText(renderBuffer, "/", 1, fontData, end, rect.y + (fontData->lineHeight / 2), textColor, editor->rect);
        }
        else
        {
            // file
            RenderText(renderBuffer, fileList->files[n].cFileName, strlen(fileList->files[n].cFileName), fontData, rect.x + 5, rect.y + (fontData->lineHeight / 2), textColor, editor->rect);
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
    
    RenderText(renderBuffer, welcomeText, strlen(welcomeText), fontData, x, y ,(Color){255, 255, 255, 255}, editor->rect);
    
    u8 *openText = "Open File - Ctrl + O\0";
    
    width = strlen(openText) * (fontData->charDatas['A'].x1 - fontData->charDatas['A'].x0);
    x = (editor->rect.width / 2) - (width / 2);
    y = (editor->rect.height / 2) + fontData->lineHeight + 4;
    
    RenderText(renderBuffer, openText, strlen(openText), fontData, x, y ,(Color){255, 255, 255, 255}, editor->rect);
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

