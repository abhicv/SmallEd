#ifndef SED_PIECE_TABLE
#define SED_PIECE_TABLE

#include "sed_types.h"

typedef enum NodeType {
    Original = 0,
    Added,
} NodeType;

typedef struct Node {
    u32 start;
    u32 length;
    NodeType type;
} Node;
 
typedef struct PieceTable
{
    u8 *original;
    u8 *added;    
    Node *nodes;
} PieceTable;

void AddNode(Node node);

#endif
