#pragma once

#include <cstdint>

// Primitive aliases
using Bitboard = int;
using Move     = int;
using Score    = int;
using Square = int;
using Piece = int;

struct UndoInfo {
    Move move;
    int castlingRights;
    int epSquare;
    int halfmoveClock;
    int capturedPiece;
    Square wkSquare;
    Square bkSquare;
};