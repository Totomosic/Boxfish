#pragma once
#include "Position.h"
#include <string>
#include <iostream>

namespace Boxfish
{

	Position CreateStartingPosition();
	Position CreatePositionFromFEN(const std::string& fen);
	std::string GetFENFromPosition(const Position& position);

	BitBoard GetTeamPiecesBitBoard(const Position& position, Team team);
	BitBoard GetOverallPiecesBitBoard(const Position& position);

	Piece GetPieceAtSquare(const Position& position, Team team, const Square& square);

	std::ostream& operator<<(std::ostream& stream, const Position& position);

}
