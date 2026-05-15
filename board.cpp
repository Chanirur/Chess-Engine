#include "board.h"
#include <cstdint>
#include <iostream>
using namespace std;

/* 
Move encoding

move & 0x3F -> from square (6 bits)
move >> 6 & 0x3F -> to square (6 bits)
move >> 12 & 0xF -> promotion piece (4 bits)
move >> 16 & 0x1 -> is capture (1 bit)
move >> 17 & 0x1 -> is en passant (1 bit)
move >> 18 & 0x1 -> is castling (1 bit)
move >> 19 & 0x1 -> is double pawn push (1 bit)
*/


void Board::loadInitialPos() {
    bitboards[0] = 0x000000000000FF00; // White pawns
    bitboards[1] = 0x0000000000000042; // White knights
    bitboards[2] = 0x0000000000000024; // White bishops
    bitboards[3] = 0x0000000000000081; // White rooks
    bitboards[4] = 0x0000000000000008; // White queen
    bitboards[5] = 0x0000000000000010; // White king
    bitboards[6] = 0x00FF000000000000; // Black pawns
    bitboards[7] = 0x4200000000000000; // Black knights
    bitboards[8] = 0x2400000000000000; // Black bishops
    bitboards[9] = 0x8100000000000000; // Black rooks
    bitboards[10] = 0x0800000000000000; // Black queen
    bitboards[11] = 0x1000000000000000; // Black king
    sideToMove = 0;
    castlingRights = 0b1111;
    epSquare = 0;
    halfmoveClock = 0;
    fullmoveNumber = 1;
    return;
}

void Board::printBoard() {
    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file < 8; file++) {
            int square = rank * 8 + file;
            char pieceChar = '.';
            for (int i = 0; i < 12; i++) {
                if (bitboards[i] & (1ULL << square)) {
                    pieceChar = "PNBRQKpnbrqk"[i];
                    break;
                }
            }
            cout << pieceChar << " ";
        }
        cout << endl;
    }
    cout << "Side to move: " << (sideToMove == 0 ? "White" : "Black") << endl;
    cout << "Castling rights: " 
         << ((castlingRights & 0b1000) ? "K" : "")
         << ((castlingRights & 0b0100) ? "Q" : "")
         << ((castlingRights & 0b0010) ? "k" : "")
         << ((castlingRights & 0b0001) ? "q" : "") 
         << endl;
    cout << "En passant square: " << epSquare << endl;
    cout << "Halfmove clock: " << halfmoveClock << endl;
    cout << "Fullmove number: " << fullmoveNumber << endl;
}

void Board::makeMove(int move) {
    for (int i = 0; i < 12; i++) {
    if (bitboards[i] & (1ULL << (move & 0x3F))) {

        bitboards[i] &= ~(1ULL << (move & 0x3F));
        bitboards[i] |=  (1ULL << (move >> 6 & 0x3F));

        break;
    }
    
}
}

void Board::undoMove() { /* restore state */ }