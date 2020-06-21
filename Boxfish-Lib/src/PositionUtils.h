#pragma once
#include "Position.h"
#include <string>

namespace Boxfish
{

	Position CreateStartingPosition();
	Position CreatePositionFromFEN(const std::string& fen);
	std::string GetFENFromPosition(const Position& position);

}
