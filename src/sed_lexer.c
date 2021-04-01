#include "sed_lexer.h"

b32 MatchString(const u8* buffer, u32 bufferSize, u32 currentIndex, const u8* matchString, u32 matchStringLen)
{
    if(currentIndex + matchStringLen > bufferSize)
    {
        return false;
    }
    
    u8 prevChar = 0;
    
    if(currentIndex > 0)
    {
        prevChar = buffer[currentIndex - 1];
    }
    
    if(!ALPHA_CHAR(prevChar) || currentIndex == 0)
    {
        for(u32 i = 0; i < matchStringLen; i++)
        {
            if(buffer[currentIndex + i] != matchString[i])
            {
                return false;
            }
        }
    }
    else
    {
        return false;
    }
    
    //last character whitespace check
    if(!ALPHA_CHAR(buffer[currentIndex + matchStringLen]) || (currentIndex + matchStringLen) == bufferSize)
    {
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
    
    for(u32 n = 0; n < bufferSize; n++)
    {
        colorIndexBuffer[n] = COLOR_INDEX_DEFAULT;
        
        if(ALPHA_CHAR(buffer[n])) //keywords
        {
            u32 keyWordCount = sizeof(KEYWORD_LIST(C)) / sizeof(u8*);
            
            for(u32 m = 0; m < keyWordCount; m++)
            {
                u8* keyWord = KEYWORD_LIST(C)[m];
                u32 keyWordLen = strlen(keyWord);
                
                if(MatchString(buffer, bufferSize, n, keyWord, keyWordLen))
                {
                    u32 startIndex = n;
                    
                    for(u32 i = startIndex; i < (n + keyWordLen); i++)
                    {
                        colorIndexBuffer[i] = COLOR_INDEX_KEYWORD;
                    }
                    n += keyWordLen - 1;
                    
                    break;
                }
            }
        }
        else if(buffer[n] == '\"') //strings
        {
            u32 startIndex = n;
            n += 1;
            
            b32 stringTokenFound = false;
            while(n < bufferSize)
            {
                if(buffer[n] == '\"')
                {
                    stringTokenFound = true;
                    break;
                }
                n++;
            }
            
            if(stringTokenFound)
            {
                for(u32 i = startIndex; i <= n; i++)
                {
                    colorIndexBuffer[i] = COLOR_INDEX_STRING;
                }
            }
        }
        else if(NUMBER_CHAR(buffer[n])) //numbers
        {
            u8 prevChar = 0;
            
            if(n > 1)
            {
                prevChar = buffer[n - 1];
            }
            
            if(ALPHA_CHAR(prevChar))
            {
                while(n < bufferSize && NUMBER_CHAR(buffer[n]))
                {
                    n++;
                }
            }
            else
            {
                u32 startIndex = n;
                
                while(n < bufferSize && NUMBER_CHAR(buffer[n]))
                {
                    n++;
                }
                
                for(u32 i = startIndex; i < n; i++)
                {
                    colorIndexBuffer[i] = COLOR_INDEX_NUMBER;
                }
            }
            
        }
        else if(buffer[n] == '\'' && buffer[n + 2] == '\'') //single character
        {
            colorIndexBuffer[n] = COLOR_INDEX_NUMBER;
            colorIndexBuffer[n + 1] = COLOR_INDEX_NUMBER;
            colorIndexBuffer[n + 2] = COLOR_INDEX_NUMBER;
            n += 3;
        }
        else if(buffer[n] == '/' && buffer[n + 1] == '/') //comments
        {
            u32 startIndex = n;
            u32 size = bufferSize - n;
            
            for(u32 i = startIndex; i < (startIndex + size); i++)
            {
                colorIndexBuffer[i] = COLOR_INDEX_COMMENT;
            }
            n += size;
        }
    }
}

