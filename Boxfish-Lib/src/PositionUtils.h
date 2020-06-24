#pragma once
#include "Position.h"
#include "Move.h"
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
	Piece GetPieceAtSquare(const Position& position, Team team, SquareIndex square);

	bool IsInCheck(const Position& position, Team team);
	bool IsSquareUnderAttack(const Position& position, Team byTeam, const Square& square);
	bool IsSquareUnderAttack(const Position& position, Team byTeam, SquareIndex square);

	void ApplyMove(Position& position, const Move& move);
	Move CreateMove(const Position& position, const Square& from, const Square& to, Piece promotionPiece = PIECE_QUEEN);

	std::ostream& operator<<(std::ostream& stream, const Position& position);

}
