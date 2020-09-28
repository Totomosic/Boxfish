#ifdef EMSCRIPTEN
#include <string>
#include <iostream>
#include "Boxfish.h"
using namespace Boxfish;

#include <emscripten/bind.h>
using namespace emscripten;

std::string Uci(const std::string& fen, int movetime)
{
	Init();
		
	Position position;
	if (fen == "startpos")
		position = CreateStartingPosition();
	else
		position = CreatePositionFromFEN(fen);

	SearchLimits limits;
	limits.Milliseconds = movetime;

	Search search(1024 * 1024, true);
	Move bestMove = search.SearchBestMove(position, limits);
	char* buffer = new char[1024];
	std::string move = UCI::FormatMove(bestMove);
	return move;
}

EMSCRIPTEN_BINDINGS(my_module) {
	function("Uci", &Uci);
}

#endif
