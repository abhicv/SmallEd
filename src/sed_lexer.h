#ifndef SED_LEXER_H
#define SED_LEXER_H

#include "sed_types.h"

#define ALPHA_CHAR(c) ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
#define NUMBER_CHAR(c) (c >= '0' && c <= '9')
#define WHITESPACE(c) (c == ' ') || (c == '\n') || (c == '\r')

#define KEYWORD_LIST(x) x##_keywords

global u8* PY_keywords[] = {
    "False\0",
    "await\0",
    "else\0",
    "import\0",
    "pass\0",
    "None\0",
    "break\0",
    "except\0",
    "in\0",
    "raise\0",
    "True\0",
    "class\0",
    "finally\0",
    "is\0",
    "return\0",
    "and\0",
    "continue\0",
    "for\0",
    "lambda\0",
    "try\0",
    "as\0",
    "def\0",
    "from\0",
    "nonlocal\0",
    "while\0",
    "assert\0",
    "del\0",
    "global\0",
    "not\0",
    "with\0",
    "async\0",
    "elif\0",
    "if\0",
    "or\0",
    "yield\0"
};

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
    {255, 255, 255, 255}, //keywords
    {77, 191, 184, 255}, //strings
    {77, 191, 184, 255}, //numbers
    {204, 190, 164, 255}, //default text 
    {78, 189, 71, 255}, //comments
};

enum ColorIndex
{
    COLOR_INDEX_KEYWORD,
    COLOR_INDEX_STRING,
    COLOR_INDEX_NUMBER,
    COLOR_INDEX_DEFAULT,
    COLOR_INDEX_COMMENT,
};

#endif //SED_LEXER_H
