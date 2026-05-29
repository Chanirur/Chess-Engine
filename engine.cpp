#include "board.h"
#include "eval.h"
#include "utility.h"

#include <cstdint>
#include <iostream>
#include <sstream>

using namespace std;

int negamax(Board& board, int depth, int alpha = -100000, int beta = 100000) {
	
	if (board.isCheckmate()) {
        return -100000 - depth;
    }

    if (board.isStalemate()) {
        return 0;
    }

    if (board.isInsufficientMaterial()) {
        return 0;
    }
	
	if (depth == 0) {
		return evaluate(board);
	}

	vector<Move> legalMoves = board.generateLegalMoves();
	if (legalMoves.empty()) {
		return evaluate(board);
	}

	int maxEval = -100000;
	for (const Move& move : legalMoves) {
		board.makeMove(move);
		int eval = -negamax(board, depth - 1, -beta, -alpha);
		board.undoMove();

		maxEval = std::max(maxEval, eval);
		alpha = std::max(alpha, eval);
		// beta line removed
		if (alpha >= beta) {
			break;
		}
	}

	return maxEval;
}

int findBestMove(Board& board, int depth) {
	vector<Move> legalMoves = board.generateLegalMoves();
	Move bestMove = legalMoves[0]; // fixed: was -1
	int alpha = -100000;
	int beta = 100000;

	for (const Move& move : legalMoves) {
		board.makeMove(move);
		int eval = -negamax(board, depth - 1, -beta, -alpha); // fixed: now passes alpha/beta
		board.undoMove();

		if (eval > alpha) {
			alpha = eval;
			bestMove = move;
		}
	}

	return bestMove;
}

void uciLoop() {
	Board board;
	string line, token;

	while (getline(cin, line)) {
		istringstream iss(line);
		iss >> token;

		if (token == "uci") {
			cout << "id name MyEngine\n";
			cout << "id author You\n";
			cout << "uciok\n";

		} else if (token == "isready") {
			cout << "readyok\n";

		} else if (token == "ucinewgame") {
			board.loadInitialPos();

		} else if (token == "position") {
			string next;
			iss >> next;

			if (next == "startpos") {
				board.loadInitialPos();
				iss >> next; // consume "moves" if present
			} else if (next == "fen") {
				string fen = "", part;
				// FEN is 6 space-separated fields
				for (int i = 0; i < 6; i++) {
					iss >> part;
					if (i > 0) fen += " ";
					fen += part;
				}
				board.loadFromFEN(fen);
				iss >> next; // consume "moves" if present
			}

			if (next == "moves") {
				string moveStr;
				while (iss >> moveStr) {
					for (const Move& m : board.generateLegalMoves()) {
						if (moveToStr(m) == moveStr) {
							board.makeMove(m);
							break;
						}
					}
				}
			}
		} else if (token == "go") {
			cerr << "side: " << board.sideToMove << " ep: " << board.epSquare << "\n";
			for (int i = 0; i < 12; i++)
				cerr << i << ": " << board.bitboards[i] << "\n";
			Move best = findBestMove(board, 5);
			cout << "bestmove " << moveToStr(best) << "\n";

		} else if (token == "quit") {
			break;
		} else if (token == "setoption") {
		}

		cout.flush();
	}
}

int test() {
	Board board;
	board.loadFromFEN("4r3/3k4/8/8/8/8/6r1/1K6 b - - 11 6");

	board.generateLegalMoves();

	for (const Move& move : board.generateLegalMoves()) {
		if (moveToStr(move) == "e8e1") {
			board.makeMove(move);
			break;
		}
	}

	board.printBoard();

	cerr << "isCheckmate: " << board.isCheckmate()	 << "\n";

//	cout << "Best move: " << moveToStr(findBestMove(board, 5)) << "\n";

	return 0;
}

int main() {
	uciLoop();
//	test();
}