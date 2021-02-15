#include "smalled_lexer.h"

b32 MatchString(const u8* buffer, u32 bufferSize, u32 *currentIndex, const u8* matchString, u32 matchStringLen)
{
    u8 prevChar = 0;
    
    if((*currentIndex) > 0)
    {
        prevChar = buffer[(*currentIndex) - 1];
    }
    
    if(!ALPHA_CHAR(prevChar) || (*currentIndex == 0))
    {
        for(u32 i = 0; i < matchStringLen; i++)
        {
            if((*currentIndex + i) < bufferSize)
            {
                if(buffer[*currentIndex + i] != matchString[i])
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }
    }
    else
    {
        return false;
    }
    
    if(!ALPHA_CHAR(buffer[*currentIndex + matchStringLen]) || ((*currentIndex + matchStringLen) == bufferSize)) //char after last char of search word
    {
        *currentIndex += matchStringLen;
        return true;
    }
    
    return false;
}

void Lexer(const u8 *buffer, u32 bufferSize, u8* colorIndexBuffer, u32 colorIndexBufferSize, b32 *multiCommentLine)
{
    //for multi line comment
    if(bufferSize >= 2)
    {
        if((buffer[0] == '/' && buffer[1] == '*'))
        {
            *multiCommentLine = true;
        }
        
        if(*multiCommentLine)
        {
            if((buffer[bufferSize - 2] == '*' && buffer[bufferSize - 1] == '/'))
            {
                for(u32 n = 0; n < bufferSize; n++)
                {
                    colorIndexBuffer[n] = COLOR_INDEX_COMMENT;
                }
                
                *multiCommentLine = false;
            }
            else
            {
                for(u32 n = 0; n < bufferSize; n++)
                {
                    colorIndexBuffer[n] = COLOR_INDEX_COMMENT;
                }
            }
            return;
        }
    }
    
    u32 keyWordCount = sizeof(C_keywords) / sizeof(u8*);
    
    for(u32 n = 0; n < bufferSize; n++)
    {
        colorIndexBuffer[n] = COLOR_INDEX_DEFAULT;
        
        if(ALPHA_CHAR(buffer[n])) //keywords
        {
            for(u32 m = 0; m < keyWordCount; m++)
            {
                if(MatchString(buffer, bufferSize, &n, C_keywords[m], strlen(C_keywords[m])))
                {
                    u32 startIndex = n - strlen(C_keywords[m]);
                    for(u32 i = startIndex; i < n; i++)
                    {
                        colorIndexBuffer[i] = COLOR_INDEX_KEYWORD;
                    }
                    break;
                }
            }
        }
        else if(buffer[n] == '\"') //strings
        {
            n += 1;
            u32 startIndex = n;
            u32 size = 0;
            
            b32 stringTokenFound = false;
            while(buffer[n] != '\"')
            {
                size++;
                n++;
                if(buffer[n] == '\n' || n == bufferSize)
                {
                    stringTokenFound = false;
                    break;
                }
                stringTokenFound = true;
            }
            
            if(stringTokenFound)
            {
                u32 startIndex = n - size - 1;
                
                for(u32 i = startIndex; i <= n; i++)
                {
                    colorIndexBuffer[i] = COLOR_INDEX_STRING;
                }
            }
        }
        else if(NUMBER_CHAR(buffer[n])) //numbers
        {
            if(n > 1)
            {
                if(!ALPHA_CHAR(buffer[n - 1]))
                {
                    while(NUMBER_CHAR(buffer[n]))
                    {
                        colorIndexBuffer[n] = COLOR_INDEX_NUMBER;
                        n++;
                    }
                }
                else
                {
                    while(NUMBER_CHAR(buffer[n]))
                    {
                        colorIndexBuffer[n] = COLOR_INDEX_DEFAULT;
                        n++;
                    }
                }
            }
        }
        
        else if(buffer[n] == '\'' && buffer[n + 2] == '\'') //single character
        {
            colorIndexBuffer[n] = COLOR_INDEX_NUMBER;
            colorIndexBuffer[n + 1] = COLOR_INDEX_NUMBER;
            colorIndexBuffer[n + 2] = COLOR_INDEX_NUMBER;
            n += 2;
        }
        else if(buffer[n] == '/' && buffer[n + 1] == '/') //comments
        {
            u32 startIndex = n;
            u32 size = 0;
            
            while(buffer[n] != '\n' && n < bufferSize)
            {
                size++;
                n++;
            }
            
            for(u32 i = startIndex; i < (startIndex + size); i++)
            {
                colorIndexBuffer[i] = COLOR_INDEX_COMMENT;
            }
        }
    }
}

