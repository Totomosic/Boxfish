#pragma once
#include "Position.h"
#include <string>
#include <iostream>

namespace Boxfish
{

	Position CreateStartingPosition();
	Position CreatePositionFromFEN(const std::string& fen);
	std::string GetFENFromPosition(const Position& position);

	std::ostream& operator<<(std::ostream& stream, const Position& position);

}
