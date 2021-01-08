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

void EditorSpaceEvent(SDL_Event *event, Editor *editor)
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
            if(editor->gotoLine)
            {
                DeleteItem(&editor->gotoLineTSeq);
            }
            else
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
        }
        else if(event->key.keysym.sym == SDLK_RETURN)
        {
            if(editor->gotoLine)
            {
                editor->gotoLine = false;
                editor->gotoLineTSeq.gapSize = editor->gotoLineTSeq.bufferCapacity;
                editor->gotoLineTSeq.preSize = 0;
                editor->gotoLineTSeq.postSize = 0;
            }
            else
            {
                InsertLine(textBuffer);
            }
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
            //goto to line
            editor->gotoLine = true;
        }
        break;
        
        case SDL_TEXTINPUT:
        
        if(editor->gotoLine)
        {
            u8 d = event->text.text[0];
            if(d >= '0' && d <= '9')
            {
                InsertItem(&editor->gotoLineTSeq, d);
            }
        }
        else
        {
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
        }
        break;
    }
}

void RenderEditor(Buffer *renderBuffer, FontData *fontData, Editor *editor)
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
        for(u32 n = 0; n < textBuffer->lines[textBuffer->currentLine].preSize; n++)
        {
            u8 c = textBuffer->lines[textBuffer->currentLine].buffer[n];
            
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
    
    textBuffer->maxLinesVisible = ((editor->rect.height - header.height) / fontData->lineHeight) + 1;
    
    //scrolling by changing lowest visible line number
    if(textBuffer->currentLine >= (textBuffer->lowestLine + textBuffer->maxLinesVisible - 1))
    {
        textBuffer->lowestLine++;
    }
    else if(textBuffer->currentLine < textBuffer->lowestLine)
    {
        textBuffer->lowestLine--;
    }
    
    u32 highestLineNumber = (textBuffer->capacity - textBuffer->postSize) + (textBuffer->maxLinesVisible - (textBuffer->currentLine - textBuffer->lowestLine));
    
    highestLineNumber = highestLineNumber < (textBuffer->capacity - 1) ? highestLineNumber : (textBuffer->capacity - 1);
    
    caret.y = caret.y + (textBuffer->currentLine - textBuffer->lowestLine) * caret.height;
    lineHighlight.y = caret.y;
    
    //Rendering
    {
        ClearBuffer(renderBuffer, (Color){4, 35, 40, 255});
        
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
        
        u8 headerText[100] = {0};
        sprintf(headerText, "%s - Row: %d Col: %d - %0.1f %%\0", editor->fileName, textBuffer->currentLine + 1, textBuffer->lines[textBuffer->currentLine].preSize + 1, percent);
        
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
                
                b32 activeLine = (n == textBuffer->currentLine) ? true : false;
                RenderTextSequence(renderBuffer, tSeq, fontData, lineMargin.width, y, activeLine);
                y += fontData->lineHeight;
                
                free(text);
            }
        }
        
        if(editor->gotoLine)
        {
            Rect gotoRect = {0};
            gotoRect.x = 0;
            gotoRect.y = header.y + header.height;
            gotoRect.width = header.width;
            gotoRect.height = header.height;
            DrawRect(renderBuffer, &gotoRect, (Color){4, 35, 40, 255});
            
            u32 end = RenderText(renderBuffer, "Goto Line:", 10, fontData, gotoRect.x + 4, gotoRect.y + 2, (Color){255, 255, 255, 255});
            RenderTextSequenceSimple(renderBuffer, &editor->gotoLineTSeq, fontData, end + 5, gotoRect.y + 2, (Color){200, 200, 200, 255});
        }
    }
}

