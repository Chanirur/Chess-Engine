#pragma once

#include "types.h"

#include <cstdint>
#include <string>
#include <vector>


struct Board {
    uint64_t bitboards[12];
    int sideToMove;
    int castlingRights;
    int epSquare;
    int halfmoveClock;
    int fullmoveNumber;
    Square wkSquare;
    Square bkSquare;
    int numPieces;

    std::vector<UndoInfo> history;

    void loadInitialPos();
    void printBoard();
    void makeMove(int move);
    void undoMove();
    std::vector<Move> generatePseudoLegalMoves();
    std::vector<Move> generateLegalMoves();
    int getPiece(int square);
    void setPiece(int square, int piece);
    void removePiece(int square);
    bool isCheckmate();
    bool isStalemate();
    bool isInsufficientMaterial();

    bool isAttacked(Square square, int colour);
    void loadFromFEN(const std::string& fen);
};