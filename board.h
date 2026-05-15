#include <cstdint>

struct Board {
    uint64_t bitboards[12];
    int sideToMove;
    int castlingRights;
    int epSquare;
    int halfmoveClock;
    int fullmoveNumber;

    void loadInitialPos();
    void printBoard();
    void makeMove(int move);
    void undoMove();
};