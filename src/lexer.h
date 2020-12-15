#ifndef LEXER_H
#define LEXER_H

#include "types.h"
#include "render.h"

enum Token
{
    TOKEN_IDENT,
    TOKEN_OPERATOR,
    TOKEN_KEYWORD,
    TOKEN_STRING,
    TOKEN_NUMBER,
    TOKEN_COMMENTS,
    TOKEN_CHARACTER,
    TOKEN_CURLY_BRACKETS,
    TOKEN_BRACKETS,
    TOKEN_SEMICOLON,
};

#define ALPHA_CHAR(c) ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
#define NUMBER_CHAR(c) (c >= '0' && c <= '9')

b32 MatchString(const u8* buffer, u32 bufferSize, u32 *currentIndex, const u8* matchString, u32 matchStringLen)
{
    u8 prevChar = 0;
    
    if(*currentIndex > 0)
    {
        prevChar = buffer[*currentIndex - 1];
    }
    
    if(!ALPHA_CHAR(prevChar))
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
    
    if(!ALPHA_CHAR(buffer[*currentIndex + matchStringLen]))
    {
        *currentIndex += matchStringLen;
        return true;
    }
    
    return false;
}

global u8* C_keywords[] = {
    "auto\0",
    "break\0",
    "case\0",
    "char\0",
    "continue\0",
    "const\0",
    "do\0",
    "default\0",
    "double\0",
    "else\0",
    "enum\0",
    "extern\0",
    "for\0",
    "if\0",
    "goto\0",
    "float\0",
    "int\0",
    "long\0",
    "register\0",
    "return\0",
    "signed\0",
    "static\0",
    "sizeof\0",
    "short\0",
    "struct\0",
    "switch\0",
    "typedef\0",
    "union\0",
    "void\0",
    "while\0",
    "volatile\0",
    "unsigned\0",
    "u8\0",
    "u16\0",
    "u32\0",
    "u64\0",
    "i8\0",
    "i16\0",
    "i32\0",
    "i64\0",
    "f32\0",
    "f64\0",
    "b32\0",
    "b64\0",
};

global Color ColorLookUpTable[] = {
    {216, 105, 19, 255}, //keywords
    {196, 234, 93, 255}, //strings
    {196, 234, 93, 255}, //numbers
    {183, 193, 158, 255}, //default 
    {32, 236, 240, 255}, //comments
};

u8* CreateString(const u8* buffer, u32 startIndex, u32 size)
{
    u8* str = (u8*)malloc(size + 1);
    u32 m = 0;
    for(u32 n = startIndex; n < (startIndex + size); n++)
    {
        str[m] = buffer[n];
        m++;
    }
    str[size] = 0;
    
    return str;
}

void Lexer(const u8 *buffer, u32 bufferSize, u8* colorIndexBuffer, u32 colorIndexBufferSize)
{
    for(u32 n = 0; n < bufferSize; n++)
    {
        if(ALPHA_CHAR(buffer[n])) //keywords
        {
            u32 s = sizeof(C_keywords) / sizeof(u8*);
            for(u32 m = 0; m < s; m++)
            {
                if(MatchString(buffer, bufferSize, &n, C_keywords[m], strlen(C_keywords[m])))
                {
                    u32 startIndex = n - strlen(C_keywords[m]);
                    for(u32 i = startIndex; i < n; i++)
                    {
                        colorIndexBuffer[i] = 0;
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
                if(buffer[n] == '\n')
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
                    colorIndexBuffer[i] = 1;
                }
            }
        }
        else if(NUMBER_CHAR(buffer[n])) //numbers
        {
            if(!ALPHA_CHAR(buffer[n - 1]))
            {
                colorIndexBuffer[n] = 2;
            }
        }
        else if(buffer[n] == '\'' && buffer[n + 2] == '\'')
        {
            colorIndexBuffer[n] = 2;
            colorIndexBuffer[n + 1] = 2;
            colorIndexBuffer[n + 2] = 2;
            n += 2;
        }
        else if(buffer[n] == '/' && buffer[n + 1] == '/')
        {
            u32 startIndex = n;
            u32 size = 0;
            
            while(buffer[n] != '\n')
            {
                size++;
                n++;
            }
            
            for(u32 i = startIndex; i < (startIndex + size); i++)
            {
                colorIndexBuffer[i] = 4;
            }
        }
        else
        {
            colorIndexBuffer[n] = 3; 
        }
    }
}

#endif //LEXER_H