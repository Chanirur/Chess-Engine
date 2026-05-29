#include "board.h"
#include <bits/stdc++.h>


int evaluate(Board& board) {


    


    int score = 0;

    const int pieceValues[6] = {100, 320, 330, 500, 900, 20000}; // Pawn, Knight, Bishop, Rook, Queen, King

    for (int square = 0; square < 64; square++) {
        int piece = board.getPiece(square);
        if (piece != -1) {
            int pieceType = piece % 6;
            int color = piece / 6;

            if (color == 0) {
                score += pieceValues[pieceType];
            } else {
                score -= pieceValues[pieceType];
            }
        }
    }

    if (board.numPieces <= 10) {
        int opponentKingSquare = board.sideToMove == 0 ? board.bkSquare : board.wkSquare;

        int opponentKingFile = opponentKingSquare % 8;
        int opponentKingRank = opponentKingSquare / 8;

        int distToEdge = std::min({opponentKingFile, 7 - opponentKingFile, opponentKingRank, 7 - opponentKingRank});
        score += (7 - distToEdge) * 10 * (14 - board.numPieces);

        int selfKingSquare = board.sideToMove == 0 ? board.wkSquare : board.bkSquare;
        int selfKingFile = selfKingSquare % 8;
        int selfKingRank = selfKingSquare / 8;

        int distBetweenKings = std::max(std::abs(opponentKingFile - selfKingFile), std::abs(opponentKingRank - selfKingRank));
        score += (7 - distBetweenKings) * 10 * (14 - board.numPieces);

        std::cerr << "Endgame bonus: " << (7 - distToEdge) * 10 * (14 - board.numPieces) + (7 - distBetweenKings) * 10 * (14 - board.numPieces) << "\n";
    }

    return score;
}