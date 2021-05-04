#ifndef SED_CONFIG_H
#define SED_CONFIG_H

#include "sed_file.h"
#include "sed_util.h"

typedef struct AppConfig
{
    b32 enableLineHighlight;
    b32 enableLineNumber;
    b32 enableHeader;
    
    u32 caretRoundness;
    u32 selectionRoundness;
    u32 caretType;
    u32 fontSize;
    
    u8 *fontFile;
    u8 *themeFile;
    u8 *userName;
    
} AppConfig;

#define MAX_STRING_MAP_SIZE 50
typedef struct StringMap
{
    u8 *attribute[MAX_STRING_MAP_SIZE];
    u8 *value[MAX_STRING_MAP_SIZE];
    u32 size;
} StringMap;

void FreeStringMap(StringMap *map)
{
    for(u32 n = 0; n < map->size; n++)
    {
        free(map->attribute[n]);
        free(map->value[n]);
    }
}

StringMap SplitIntoStringMap(u8 *buffer, u32 bufferSize)
{
    StringMap map = {0};
    u32 count = 0;
    u32 n = 0;
    
    while(n < (bufferSize - 1))
    {
        b32 attribFound = false;
        
        //find attribute string
        {
            u32 startIndex = n;
            u32 size = 0;
            
            while(buffer[n] != '\n' && buffer[n] != 0)
            {
                if(buffer[n] == '=')
                {
                    attribFound = true;
                    break;
                }
                if(buffer[n] != ' ')
                {
                    size++;
                }
                n++;
            }
            
            if(attribFound)
            {
                if(size > 0)
                {
                    map.attribute[count] = (u8*)malloc(size + 1);
                    u8 *attrib = map.attribute[count];
                    attrib[size] = 0;
                    memcpy(attrib, buffer+startIndex, size);
                    //printf("attrb: %s", attrib);
                }
                else
                {
                    attribFound = false;
                }
            }
        }
        
        n += 1;
        
        //find value string
        if(attribFound)
        {
            u32 startIndex = n;
            u32 size = 0;
            
            while(buffer[n] != '\n' && buffer[n] != 0)
            {
                size++;
                n++;
            }
            
            if(size > 0)
            {
                map.value[count] = (u8*)malloc(size + 1);
                u8 *value = map.value[count];
                value[size] = 0;
                memcpy(value, buffer+startIndex, size);
                //printf(" value: %s\n", value);
            }
            n += 1;
            count++;
        }
    }
    
    map.size = count;
    
    return map;
}

b32 GetAttribIndexFromStringMap(StringMap *map, u8 *attrib, u32 *index)
{
    for(u32 n = 0; n < map->size; n++)
    {
        if(!strcmp(map->attribute[n], attrib))
        {
            *index = n;
            return true;
        }
    }
    return false;
}

AppConfig ParseAppConfigFile(const u8 *appConfigFileName)
{
    AppConfig result = {0};
    
    File appConfig = ReadFileNullTerminate(appConfigFileName);
    
    if(appConfig.loaded)
    {
        //split all attribute into a Map(String::String)
        StringMap map = SplitIntoStringMap(appConfig.buffer, appConfig.size);
        
        u32 index = 0;
        if(GetAttribIndexFromStringMap(&map, "\0", &index))
        {
            
        }
        
        FreeStringMap(&map);
    }
    else
    {
        printf("Error opening App config file!!\n");
    }
    
    return result;
}

#endif //SED_CONFIG_H
