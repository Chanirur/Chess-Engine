#include "types.h"
#include "utility.h"
#include <string>

std::string moveToStr(Move move) {
    std::string str;

    Square fromSquare = move & 0x3F;
    Square toSquare = (move >> 6) & 0x3F;
    Square promotionPiece = (move >> 12) & 0xF;
    bool isCapture = ((move >> 16) & 1) != 0;
    bool isEnPassant = ((move >> 17) & 1) != 0;
    bool isCastling = ((move >> 18) & 1) != 0;
    bool isDoublePush = ((move >> 19) & 1) != 0;

    char fromFile = 'a' + (fromSquare % 8);
    char fromRank = '1' + (fromSquare / 8);
    char toFile = 'a' + (toSquare % 8);
    char toRank = '1' + (toSquare / 8);

    str += fromFile;
    str += fromRank;
    str += toFile;
    str += toRank;

    if (promotionPiece) {
        const char promChars[5] = {0, 'n', 'b', 'r', 'q'};
        if (promotionPiece < 5) {
            str += promChars[promotionPiece];
        }
    }

    return str;
}