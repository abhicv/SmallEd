#include "sed_piece_table.h"

void AppendNode(PieceTable *pieceTable, Node *node) {
    if (pieceTable->nodes == 0) {
        pieceTable->nodes = node;
    }
}

PieceTable InitializePieceTable(u8* originalBuffer) {

    PieceTable pieceTable = {0};
    pieceTable.original = originalBuffer;

    Node *node = (Node*)AllocatePersistentMemory(sizeof(Node));
    node->type = Original;
    node->parent = 0;
    node->child = 0;
    node->start = 0;
    node->length = strlen(originalBuffer);

    AppendNode(&pieceTable, node);

    return pieceTable;
}

void SplitCurrentAndInsertNode(PieceTable *pieceTable, Node *node) {
    
}
