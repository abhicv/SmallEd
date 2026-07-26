#ifndef SED_PIECE_TABLE
#define SED_PIECE_TABLE

#include "sed_types.h"
#include "sed_memory.h"

typedef enum NodeType
{
    Original = 0,
    Added,
} NodeType;

typedef struct Node
{
    u32 start;
    u32 length;
    NodeType type;
    struct Node *parent;
    struct Node *child;
} Node;

typedef struct PieceTable
{
    u8 *original;
    u8 *added;
    Node *nodes;
} PieceTable;

#endif
