#include <iostream>
#include <cstdint>
#include "board.h"
using namespace std;

int main() {
    Board board;
    board.loadInitialPos();
    board.makeMove(11 | 27 << 6);
    board.printBoard();
}