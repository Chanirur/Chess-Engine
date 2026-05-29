#include "board.h"

#include "types.h"

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

/* ;
Move encoding;

move & 0x3F -> from square (6 bits);
move >> 6 & 0x3F -> to square (6 bits);
move >> 12 & 0xF -> promotion piece (4 bits);
move >> 16 & 0x1 -> is capture (1 bit);
move >> 17 & 0x1 -> is en passant (1 bit);
move >> 18 & 0x1 -> is castling (1 bit);
move >> 19 & 0x1 -> is double pawn push (1 bit);
*/
;

void Board::loadInitialPos() {
	bitboards[0] = 0x000000000000FF00;  // White pawns;
	bitboards[1] = 0x0000000000000042;  // White knights;
	bitboards[2] = 0x0000000000000024;  // White bishops;
	bitboards[3] = 0x0000000000000081;  // White rooks;
	bitboards[4] = 0x0000000000000008;  // White queen;
	bitboards[5] = 0x0000000000000010;  // White king;
	bitboards[6] = 0x00FF000000000000;  // Black pawns;
	bitboards[7] = 0x4200000000000000;  // Black knights;
	bitboards[8] = 0x2400000000000000;  // Black bishops;
	bitboards[9] = 0x8100000000000000;  // Black rooks;
	bitboards[10] = 0x0800000000000000; // Black queen;
	bitboards[11] = 0x1000000000000000; // Black king;
	sideToMove = 0;
	castlingRights = 0b1111;
	epSquare = -1;
	halfmoveClock = 0;
	fullmoveNumber = 1;
	wkSquare = 4;
	bkSquare = 60;
	return;
}

void Board::loadFromFEN(const string& fen) {
    for (int i = 0; i < 12; i++) bitboards[i] = 0ULL;
    history.clear();

    istringstream ss(fen);
    string pieces, side, castling, ep;
    int halfmove, fullmove;
    ss >> pieces >> side >> castling >> ep >> halfmove >> fullmove;

    // FEN piece placement starts at a8 (square 56) and goes left-to-right, top-to-bottom
    int rank = 7, file = 0;
    for (char c : pieces) {
        if (c == '/') {
            rank--;
            file = 0;
        } else if (isdigit(c)) {
            file += (c - '0');
        } else {
            int square = rank * 8 + file;
            const string fenChars = "PNBRQKpnbrqk";
            int idx = fenChars.find(c);
            if (idx != (int)string::npos)
                bitboards[idx] |= (1ULL << square);
            file++;
        }
    }

    sideToMove = (side == "w") ? 0 : 1;

    castlingRights = 0;
    for (char c : castling) {
        if      (c == 'K') castlingRights |= 0b1000;
        else if (c == 'Q') castlingRights |= 0b0100;
        else if (c == 'k') castlingRights |= 0b0010;
        else if (c == 'q') castlingRights |= 0b0001;
    }

    epSquare = -1;
    if (ep != "-") {
        int epFile = ep[0] - 'a';
        int epRank = ep[1] - '1';
        epSquare = epRank * 8 + epFile;
    }

    halfmoveClock = halfmove;
    fullmoveNumber = fullmove;

    // Derive king squares directly from the bitboards we just populated
    wkSquare = bitboards[5] ? __builtin_ctzll(bitboards[5]) : 4;
    bkSquare = bitboards[11] ? __builtin_ctzll(bitboards[11]) : 60;
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
	cout << "Castling rights: " << ((castlingRights & 0b1000) ? "K" : "")
	     << ((castlingRights & 0b0100) ? "Q" : "") << ((castlingRights & 0b0010) ? "k" : "")
	     << ((castlingRights & 0b0001) ? "q" : "") << endl;
	cout << "En passant square: " << epSquare << endl;
	cout << "Halfmove clock: " << halfmoveClock << endl;
	cout << "Fullmove number: " << fullmoveNumber << endl;
}

void Board::setPiece(int square, int piece) {
	if (piece == 0) {
		for (int i = 0; i < 12; i++) {
			bitboards[i] &= ~(1ULL << square);
		}
	} else {
		bitboards[piece - 1] |= (1ULL << square);
	}
}

void Board::removePiece(int square) {
	for (int i = 0; i < 12; i++) {
		bitboards[i] &= ~(1ULL << square);
	}
}
void Board::makeMove(int move) {
	UndoInfo undoInfo;
	undoInfo.move = move;
	undoInfo.castlingRights = castlingRights;
	undoInfo.epSquare = epSquare;
	undoInfo.halfmoveClock = halfmoveClock;
	undoInfo.capturedPiece = getPiece((move >> 6) & 0x3F);
	undoInfo.wkSquare = wkSquare;
	undoInfo.bkSquare = bkSquare;

	bool halfmoveClockReset = false;

	if (move & (1 << 18)) {
		// Castling
		removePiece(move & 0x3F);
		setPiece(
		    (move >> 6) & 0x3F,
		    (sideToMove ? 12 : 6)); // Move king (piece id: white=6, black=12 in 1-based setPiece)

		if ((move & 0x3F) > ((move >> 6) & 0x3F)) {
			// Queenside
			removePiece(sideToMove == 0 ? 0 : 56); // Remove rook from a1/a8
			setPiece((sideToMove == 0 ? 3 : 59), (sideToMove == 0 ? 4 : 10)); // Move rook to d1/d8
		} else {
			// Kingside
			removePiece(sideToMove == 0 ? 7 : 63); // Remove rook from h1/h8
			setPiece(sideToMove == 0 ? 5 : 61, (sideToMove == 0 ? 4 : 10)); // Move rook to f1/f8
		}

		// Update king square
		if (sideToMove == 0) {
			wkSquare = (move >> 6) & 0x3F;
		} else {
			bkSquare = (move >> 6) & 0x3F;
		}

		// FIX BUG 6: clear correct castling rights bits
		// castlingRights layout: bit3=K(white kingside), bit2=Q(white queenside),
		//                        bit1=k(black kingside), bit0=q(black queenside)
		castlingRights &= ~(sideToMove == 0 ? 0b1100 : 0b0011);

		epSquare = -1;

	} else if (move & (1 << 17)) {
		// En passant
		removePiece(move & 0x3F);
		setPiece((move >> 6) & 0x3F,
		         sideToMove ? 7 : 1); // Move pawn (1-based: white pawn=1, black pawn=7)
		removePiece(((move >> 6) & 0x3F) + (sideToMove ? 8 : -8)); // Remove captured pawn
		halfmoveClockReset = true;
		epSquare = -1;
		numPieces--;

	} else {
		int from_square = move & 0x3F;
		int to_square = (move >> 6) & 0x3F;
		int promotion_piece = (move >> 12) & 0x7;

		if (move & (1 << 16)) {
			halfmoveClockReset = true;
			undoInfo.capturedPiece = getPiece(to_square);
			removePiece(to_square);
			numPieces--;
		}

		int piece = getPiece(from_square); // 0-based index into bitboards
		removePiece(from_square);
		if (promotion_piece) {
			// promotion_piece: 1=N,2=B,3=R,4=Q; setPiece uses 1-based piece ids
			// white: N=2,B=3,R=4,Q=5; black: N=8,B=9,R=10,Q=11
			setPiece(to_square, sideToMove ? promotion_piece + 7 : promotion_piece + 1);
		} else {
			setPiece(to_square, piece + 1); // convert 0-based index to 1-based setPiece id
		}

		// Track king position and revoke castling rights on king move
		// piece is 0-based: white king=5, black king=11
		if (piece == 5) { // white king
			wkSquare = to_square;
			// FIX BUG 6: white moves revoke white castling rights (bits 3,2)
			castlingRights &= ~0b1100;
		} else if (piece == 11) { // black king
			bkSquare = to_square;
			// FIX BUG 6: black moves revoke black castling rights (bits 1,0)
			castlingRights &= ~0b0011;
		}

		// Revoke castling rights if rook moves from its starting square
		if (from_square == 0) castlingRights &= ~0b0100;  // white queenside rook
		if (from_square == 7) castlingRights &= ~0b1000;  // white kingside rook
		if (from_square == 56) castlingRights &= ~0b0001; // black queenside rook
		if (from_square == 63) castlingRights &= ~0b0010; // black kingside rook

		// Revoke castling rights if rook is captured on its starting square
		if (to_square == 0) castlingRights &= ~0b0100;
		if (to_square == 7) castlingRights &= ~0b1000;
		if (to_square == 56) castlingRights &= ~0b0001;
		if (to_square == 63) castlingRights &= ~0b0010;

		// Pawn moves reset halfmove clock
		if (piece == 0 || piece == 6) halfmoveClockReset = true;

		if (move & (1 << 19)) {
			epSquare = to_square + (sideToMove == 0 ? -8 : 8);
		} else {
			epSquare = -1;
		}
	}

	sideToMove ^= 1;

	if (halfmoveClockReset) {
		halfmoveClock = 0;
	} else {
		halfmoveClock++;
	}

	if (!sideToMove) { // white to play because we just toggled sideToMove
		fullmoveNumber++;
	}

	history.push_back(undoInfo);
}

int Board::getPiece(int square) {
	for (int i = 0; i < 12; i++) {
		if (bitboards[i] & (1ULL << square)) {
			return i;
		}
	}
	return -1; // No piece on this square;
}

void Board::undoMove() {
	if (history.empty()) return;
	UndoInfo undoInfo = history.back();
	history.pop_back();

	int move = undoInfo.move;

	if (move & (1 << 18)) {
		// Undo castling
		if (sideToMove == 1) {
			// sideToMove was toggled, so sideToMove==1 means white just castled
			removePiece((move >> 6) & 0x3F);
			setPiece(move & 0x3F, 6);
			if ((move & 0x3F) > ((move >> 6) & 0x3F)) {
				// Queenside: rook went to d1=3
				removePiece(3);
				setPiece(0, 4); // rook back to a1
			} else {
				// Kingside: rook went to f1=5
				removePiece(5);
				setPiece(7, 4); // rook back to h1
			}
		} else {
			// Black just castled
			removePiece((move >> 6) & 0x3F); // remove king from destination
			setPiece(move & 0x3F, 12);       // restore black king (1-based id 12)
			if ((move & 0x3F) > ((move >> 6) & 0x3F)) {
				// Queenside: rook went to d8=59
				removePiece(59);
				setPiece(56, 10); // rook back to a8
			} else {
				// Kingside: rook went to f8=61
				removePiece(61);
				setPiece(63, 10); // rook back to h8
			}
		}
	} else if (move & (1 << 17)) {
		int mover = !sideToMove; // side that made the move
		removePiece((move >> 6) & 0x3F);
		setPiece(move & 0x3F, mover ? 7 : 1); // restore moving pawn (1-based)
		// Restore captured pawn on the rank it was on (not the ep square)
		int capturedPawnSquare = ((move >> 6) & 0x3F) + (mover ? 8 : -8);
		setPiece(capturedPawnSquare, mover ? 1 : 7); // opponent pawn
		numPieces++;
	} else {
		int from_square = move & 0x3F;
		int to_square = (move >> 6) & 0x3F;
		int promotion_piece = (move >> 12) & 0x7;

		int piece = getPiece(to_square); // 0-based
		removePiece(to_square);

		if (promotion_piece) {
			// Restore the original pawn
			int mover = !sideToMove;
			setPiece(from_square, mover ? 7 : 1); // 1-based pawn id
		} else {
			setPiece(from_square, piece + 1); // restore piece (1-based)
		}

		if (undoInfo.capturedPiece != -1) {
			setPiece(to_square, undoInfo.capturedPiece + 1); // restore captured piece (1-based)
			numPieces++;
		}
	}

	// Restore state
	sideToMove = !sideToMove;
	castlingRights = undoInfo.castlingRights;
	epSquare = undoInfo.epSquare;
	halfmoveClock = undoInfo.halfmoveClock;

	if (sideToMove == 1) {
		fullmoveNumber--;
	}

	wkSquare = undoInfo.wkSquare;
	bkSquare = undoInfo.bkSquare;
}

std::vector<Move> Board::generatePseudoLegalMoves() {
	std::vector<Move> moves;

	for (int i = (sideToMove ? 6 : 0); i <= (sideToMove ? 11 : 5); i++) {
		switch (i) {
			case 0:
			case 6: {
				for (int j = 0; j < 64; j++) {
					if (!(bitboards[i] & (1ULL << j))) continue;

					// En passant
					if (epSquare >= 0 && (epSquare == j + (sideToMove ? -9 : 9) ||
					                      epSquare == j + (sideToMove ? -7 : 7))) {
						int fileDiff = (epSquare % 8) - (j % 8);
						if ((fileDiff < 0 ? -fileDiff : fileDiff) == 1)
							moves.push_back(j | epSquare << 6 | (1ULL << 17));
					}

					// Pawn captures
					for (int k = 0; k < 2; k++) {
						int toSquare = j + (sideToMove ? (k == 0 ? -9 : -7) : (k == 0 ? 9 : 7));
						if (toSquare < 0 || toSquare >= 64) continue;
						int fileDiff = (toSquare % 8) - (j % 8);
						if ((fileDiff < 0 ? -fileDiff : fileDiff) > 1) continue;
						Piece target = getPiece(toSquare);
						if (target != -1 && target >= (sideToMove ? 0 : 6) &&
						    target <= (sideToMove ? 5 : 11))
							moves.push_back(j | toSquare << 6 | (1ULL << 16));
					}

					// Forward push blocked?
					int oneStep = j + (sideToMove ? -8 : 8);
					if (getPiece(oneStep) != -1) continue;

					// Promotion: white on rank 6 (sq 48-55) pushes to rank 7; black on rank 1 (sq
					// 8-15) pushes to rank 0
					if ((sideToMove == 0 && j >= 48 && j < 56) ||
					    (sideToMove == 1 && j >= 8 && j < 16)) {
						for (int promo = 1; promo <= 4; promo++)
							moves.push_back(j | oneStep << 6 | promo << 12);
					} else {
						// Single push
						moves.push_back(j | oneStep << 6);
						// Double push from starting rank
						if ((sideToMove == 0 && j >= 8 && j < 16) ||
						    (sideToMove == 1 && j >= 48 && j < 56)) {
							int twoStep = j + (sideToMove ? -16 : 16);
							if (getPiece(twoStep) == -1)
								moves.push_back(j | twoStep << 6 | (1ULL << 19));
						}
					}
				}
			}; break;

			case 1:
			case 7: {


				for (int j = 0; j < 64; j++) {

					if (!(bitboards[i] & (1ULL << j))) continue;

					int knightMoves[8] = {17, 15, 10, 6, -17, -15, -10, -6};
					for (int k = 0; k < 8; k++) {
						int toSquare = j + knightMoves[k];
						if (toSquare < 0 || toSquare >= 64) continue;

						int fileDiff = (toSquare % 8) - (j % 8);
						if ((fileDiff < 0 ? -fileDiff : fileDiff) > 2) continue;

						
						Piece target = getPiece(toSquare);

						if (target == -1) {

							moves.push_back(j | toSquare << 6);
						} else if (target >= (sideToMove ? 0 : 6) && target <= (sideToMove ? 5 : 11)) {
							moves.push_back(j | toSquare << 6 | (1ULL << 16));
						}
					}
				}
			}; break;

			// 2,8 - Bishop, 3,9 - Rook, 4,10 - Queen
			case 2:
			case 8:
			case 3:
			case 9:
			case 4:
			case 10: {
				// directions: indices 0-3 = diagonals (bishop), 4-7 = straights (rook)
				int8_t directions[8] = {9, 7, -9, -7, 8, 1, -8, -1};

				// FIX BUG 1: correct direction index ranges per piece type
				int dirStart = (i == 2 || i == 8) ? 0 : (i == 3 || i == 9) ? 4 : 0;
				int dirEnd = (i == 2 || i == 8) ? 4 : 8;

				for (int j = dirStart; j < dirEnd; j++) {
					for (int k = 0; k < 64; k++) {
						if (!(bitboards[i] & (1ULL << k))) continue;
						Square toSquare = k + directions[j];
						while (0 <= toSquare && toSquare < 64) {
							// FIX BUG 2: check wrap between consecutive squares, not from origin
							int prevSquare = toSquare - directions[j];
							int fileDiff = (toSquare % 8) - (prevSquare % 8);
							if ((fileDiff < 0 ? -fileDiff : fileDiff) > 1) break;

							Piece target = getPiece(toSquare);
							if (target == -1) {
								moves.push_back(k | toSquare << 6);
							} else {
								if (target >= (sideToMove ? 0 : 6) &&
								    target <= (sideToMove ? 5 : 11)) {
									moves.push_back(k | toSquare << 6 | (1ULL << 16));
								}
								break; // blocked regardless
							}
							toSquare += directions[j];
						}
					}
				}

			}; break;
			case 5:
			case 11: {
				int8_t kingMoves[8] = {8, 9, 1, -7, -8, -9, -1, 7};

				for (int j = 0; j < 64; j++) {
					if (!(bitboards[i] & (1ULL << j))) continue;

					for (int k = 0; k < 8; k++) {
						Square toSquare = j + kingMoves[k];

						// FIX BUG 3: was "0 >= toSquare" which skips square 0 and allows square 64
						if (toSquare < 0 || toSquare >= 64) continue;
						int fileDiff = (toSquare % 8) - (j % 8);
						if ((fileDiff < 0 ? -fileDiff : fileDiff) > 1) continue;

						Piece target = getPiece(toSquare);
						if (target == -1) {
							moves.push_back(j | toSquare << 6);
						} else {
							if (target >= (sideToMove ? 0 : 6) && target <= (sideToMove ? 5 : 11)) {
								moves.push_back(j | toSquare << 6 | (1ULL << 16));
							} else
								continue;
						}
					}
				};
				break;
			}
		}

		if (castlingRights & (1ULL << (sideToMove ? 0 : 2))) {
			Square square = sideToMove ? bkSquare : wkSquare;

			if (getPiece(square - 1) == -1 && getPiece(square - 2) == -1 &&
			    getPiece(square - 3) == -1) {
				moves.push_back(square | ((square - 2) << 6) | (1ULL << 18));
			}
		}

		
	}
	
	if (castlingRights & (1ULL << (sideToMove ? 1 : 3))) {
			// Kingside Castle
			Square square = sideToMove ? bkSquare : wkSquare;

			if (getPiece(square + 1) == -1 && getPiece(square + 2) == -1) {
				moves.push_back(square | ((square + 2) << 6) | (1ULL << 18));
			}
		}
	return moves;
}

bool Board::isAttacked(Square square, int colour) {
	int pawn_offsets[2] = {(colour == 0 ? -7 : 7), (colour == 0 ? -9 : 9)};
	for (int i = 0; i < 2; i++) {
		Square target = square + pawn_offsets[i];

		if (target < 0 || target >= 64) continue;
		int fileDiff = (target % 8) - (square % 8);
		if ((fileDiff < 0 ? -fileDiff : fileDiff) > 1) continue;
		if (bitboards[(colour ? 6 : 0)] & (1ULL << target)) {
			return true;
		}
	};

	int8_t knight_offsets[8] = {17, 15, 10, 6, -17, -15, -10, -6};
	for (int i = 0; i < 8; i++) {
		int target = square + knight_offsets[i];
		if (target < 0 || target >= 64) continue;
		int fileDiff = (target % 8) - (square % 8);
		if ((fileDiff < 0 ? -fileDiff : fileDiff) > 2) continue;
		if (bitboards[(colour ? 7 : 1)] & (1ULL << target)) {
			return true;
		}
	};

	int8_t const queenOffsets[8] = {7, 9, -7, -9, 8, 1, -8, -1};

	for (int i = 0; i < 8; i++) {
		Square target = square + queenOffsets[i];

		while (0 <= target && target < 64) {
			int prevTarget = target - queenOffsets[i];
			int fileDiff = (target % 8) - (prevTarget % 8);
			int expectedFileDiff = (i < 4 || i == 5 || i == 7) ? 1 : 0;
			if ((fileDiff < 0 ? -fileDiff : fileDiff) > expectedFileDiff) break;

			if (bitboards[colour ? 10 : 4] & (1ULL << target)) {
				return true;
			}
			if (i < 4 && bitboards[colour ? 8 : 2] & (1ULL << target)) {
				return true;
			}
			if (i >= 4 && bitboards[colour ? 9 : 3] & (1ULL << target)) {
				return true;
			}

			if (getPiece(target) != -1) break;

			target += queenOffsets[i];
		}
	}

	int8_t const kingOffsets[8] = {8, 9, 1, -7, -8, -9, -1, 7};
	for (int i = 0; i < 8; i++) {
		int target = square + kingOffsets[i];
		if (target < 0 || target >= 64) continue;
		int fileDiff = (target % 8) - (square % 8);
		if (((fileDiff < 0 ? -fileDiff : fileDiff)) > 1) continue;
		if (bitboards[(colour ? 11 : 5)] & (1ULL << target)) {
			return true;
		}
	}

	return false;
}

std::vector<Move> Board::generateLegalMoves() {
	std::vector<Move> pseudoLegalMoves = generatePseudoLegalMoves();

	std::vector<Move> legalMoves = {};

	for (int i = 0; i < (int)pseudoLegalMoves.size(); i++) {
		makeMove(pseudoLegalMoves[i]);

		if (pseudoLegalMoves[i] & (1 << 18)) {
			int from_square = pseudoLegalMoves[i] & 0x3F;
			int to_square = (pseudoLegalMoves[i] >> 6) & 0x3F;
			int step = to_square > from_square ? 1 : -1;

			if (!isAttacked(from_square, sideToMove) &&
			    !isAttacked(from_square + step, sideToMove) && !isAttacked(to_square, sideToMove)) {
				legalMoves.push_back(pseudoLegalMoves[i]);
			}
			undoMove();
			continue;
		}

		Square kingSquare = sideToMove == 0 ? bkSquare : wkSquare;
		if (!isAttacked(kingSquare, sideToMove)) {
			legalMoves.push_back(pseudoLegalMoves[i]);
		}
		undoMove();
	}

	return legalMoves;
}

bool Board::isCheckmate() {
	Square kingSquare = sideToMove ? bkSquare : wkSquare;
	return generateLegalMoves().empty() && isAttacked(kingSquare, sideToMove == 0 ? 1 : 0);
}

bool Board::isStalemate() {
	Square kingSquare = sideToMove ? bkSquare : wkSquare;
	return generateLegalMoves().empty() && !isAttacked(kingSquare, sideToMove == 0 ? 1 : 0);
}

bool Board::isInsufficientMaterial() {
	for (int i = 0; i < 12; i++) {
		if (bitboards[i] != 0) {
			if (i == 0 || i == 6 || i == 3 || i == 9 || i == 4 || i == 10) {
				return false;
			}
		}
	}

	int whiteBishops = __builtin_popcountll(bitboards[2]);
	int blackBishops = __builtin_popcountll(bitboards[8]);
	int whiteKnights = __builtin_popcountll(bitboards[1]);
	int blackKnights = __builtin_popcountll(bitboards[7]);

	if (whiteBishops + blackBishops + whiteKnights + blackKnights > 1) {
		return false;
	}

	return true;
}