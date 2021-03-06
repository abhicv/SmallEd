#include "sed_editor.h"
#include "sed_util.h"

//EVENT HANDLING FUNCTIONS
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
                textBuffer->desiredCursorPos = tSeq->preSize;
            }
            else
            {
                DeleteLine(textBuffer);
                textBuffer->desiredCursorPos = editor->textBuffer.lines[textBuffer->currentLine].preSize;
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
        else if(ctrlDown && event->key.keysym.sym == SDLK_s) //save file
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
        
        case SDL_MOUSEWHEEL:
        if(event->wheel.y > 0) // scroll up
        {
            MoveCursorUp(textBuffer);
        }
        else if(event->wheel.y < 0) // scroll down
        {
            MoveCursorDown(textBuffer);
        }
        break;
        
        case SDL_TEXTINPUT:
        InsertItem(tSeq, event->text.text[0]);
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

//RENDERING FUNCTIONS

void EditorUpdateAndRender(Buffer *renderBuffer, FontData *fontData, Editor *editor)
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
    lineMargin.width = 10 + DigitCount(textBuffer->preSize + textBuffer->postSize) * fontData->charDatas[(u32)('0')].xadvance;
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
    
    caret.y = header.y + header.height + ((textBuffer->currentLine - textBuffer->lowestLine) * caret.height);
    lineHighlight.y = caret.y;
    
    //Rendering
    {
        //header
        DrawRect(renderBuffer, &header, (Color){214, 181, 139, 255});
        
        //line margin
        DrawRect(renderBuffer, &lineMargin, (Color){4, 35, 40, 255});
        
        //line highlight
        DrawRect(renderBuffer, &lineHighlight, (Color){28, 60, 55, 255});
        
        //caret(cursor)
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
                
                //highlighting current line number
                if(n == textBuffer->currentLine)
                {
                    RenderText(renderBuffer, lineNumText, nDigits, fontData, lineMargin.x + lineMargin.width - x, y, (Color){255, 255, 0, 255}, editor->rect);
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
    
    //editor space border
    DrawRectWire(renderBuffer, &editor->rect, (Color){255, 255, 255, 255});
}

void RenderFileLister(Buffer *renderBuffer, FontData *fontData, Editor *editor)
{
    FileList *fileList = &editor->fileList;
    Color textColor = {204, 190, 164, 255};
    
    //current directory header
    Rect headerRect = {0};
    headerRect.x = editor->rect.x;
    headerRect.y = editor->rect.y;
    headerRect.width = editor->rect.width;
    headerRect.height = 1.5f * fontData->lineHeight;
    
    DrawRectWire(renderBuffer, &headerRect, (Color){214, 181, 139, 255});
    
    u32 endX = RenderText(renderBuffer, "Open:\0", strlen("Open:\0"), fontData, 
                          headerRect.x + 5, headerRect.y  + (headerRect.height - fontData->lineHeight) / 2, 
                          (Color){255, 255, 0, 255}, editor->rect);
    
    RenderText(renderBuffer, fileList->currentDir, strlen(fileList->currentDir), fontData, 
               endX + 10, headerRect.y  + (headerRect.height - fontData->lineHeight) / 2, 
               textColor, editor->rect);
    
    Rect rect = {0};
    rect.x = editor->rect.x;
    rect.y = editor->rect.y + headerRect.height;
    rect.width = editor->rect.width;
    rect.height = 2 * fontData->lineHeight;
    
    
    for(u32 n = 2; n < fileList->fileCount; n++)
    {
        if(n == fileList->selectedIndex)
        {
            DrawRectWire(renderBuffer, &rect, (Color){214, 181, 139, 255});
            //DrawRect(renderBuffer, &rect, (Color){214, 181, 139, 255});
            textColor = (Color){255, 255, 0, 255};
            //textColor = (Color){0, 0, 0, 255};
        }
        else
        {
            //DrawRectWire(renderBuffer, &rect, (Color){214, 181, 139, 255});
            textColor = (Color){204, 190, 164, 255};
        }
        
        if(fileList->files[n].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            // directory
            
            //icon rendering
            u32 end = RenderText(renderBuffer, "d\0", 1, fileList->fileIconFont, rect.x + 10, rect.y + (rect.height - fileList->fileIconFont->lineHeight) / 2, 
                                 textColor, editor->rect);
            
            end = RenderText(renderBuffer, fileList->files[n].cFileName, 
                             strlen(fileList->files[n].cFileName), fontData, end + 15, rect.y + (fontData->lineHeight / 2), 
                             textColor, editor->rect);
            //RenderText(renderBuffer, "\\\0", 1, fontData, end, rect.y + (fontData->lineHeight / 2), textColor, editor->rect);
        }
        else
        {
            // file
            
            //icon rendering
            u32 end  = RenderText(renderBuffer, "f\0", 1, fileList->fileIconFont, rect.x + 10, rect.y + (rect.height - fileList->fileIconFont->lineHeight) / 2, 
                                  textColor, editor->rect);
            
            RenderText(renderBuffer, fileList->files[n].cFileName, strlen(fileList->files[n].cFileName), fontData, end + 15, rect.y + (fontData->lineHeight / 2), textColor, editor->rect);
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
    
    RenderText(renderBuffer, welcomeText, strlen(welcomeText), fontData, x, y, (Color){255, 255, 255, 255}, editor->rect);
    
    u8 *openText = "Open File - Ctrl + O\0";
    
    width = strlen(openText) * (fontData->charDatas['A'].x1 - fontData->charDatas['A'].x0);
    x = (editor->rect.width / 2) - (width / 2);
    y = (editor->rect.height / 2) + fontData->lineHeight + 4;
    
    u32 end = RenderText(renderBuffer, openText, strlen("Open File - \0"), fontData, x, y, (Color){255, 255, 255, 255}, editor->rect);
    RenderText(renderBuffer, "Ctrl + O\0", strlen("Ctrl + O\0"), fontData, end, y, (Color){255, 255, 0, 255}, editor->rect);
}

void AppUpdateAndRender(Buffer *renderBuffer, FontData *fontData, Editor *editor)
{
    switch(editor->mode)
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

