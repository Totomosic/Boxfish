#pragma once
#include "Position.h"
#include "Move.h"
#include <string>
#include <iostream>

namespace Boxfish
{

	struct BOX_API UndoInfo
	{
	public:
		int HalfTurnsSinceCaptureOrPush = 0;
		Square EnpassantSquare = INVALID_SQUARE;
		bool CastleKingSide[TEAM_MAX];
		bool CastleQueenSide[TEAM_MAX];
	};

	Position CreateStartingPosition();
	Position CreatePositionFromFEN(const std::string& fen);
	std::string GetFENFromPosition(const Position& position);

	BitBoard GetTeamPiecesBitBoard(const Position& position, Team team);
	BitBoard GetOverallPiecesBitBoard(const Position& position);

	BitBoard GetAttackers(const Position& position, Team team, const Square& square, const BitBoard& blockers);
	BitBoard GetAttackers(const Position& position, Team team, SquareIndex square, const BitBoard& blockers);
	// Returns a bitboard containing all the pieces that are pinned by sliders to the square
	// 'sliders' is a bitboard containing a superset of all sliding pieces to check against
	BitBoard GetSliderBlockers(const Position& position, const BitBoard& sliders, SquareIndex square, BitBoard* pinners);
	Team GetTeamAt(const Position& position, SquareIndex square);

	void CalculateKingBlockers(Position& position, Team team);
	void CalculateCheckers(Position& position, Team team);
	void CalculateKingSquare(Position& position, Team team);

	bool IsSquareOccupied(const Position& position, Team team, const Square& square);
	bool IsSquareOccupied(const Position& position, Team team, SquareIndex square);
	Piece GetPieceAtSquare(const Position& position, Team team, const Square& square);
	Piece GetPieceAtSquare(const Position& position, Team team, SquareIndex square);
	bool IsPieceOnSquare(const Position& position, Team team, Piece piece, SquareIndex square);

	bool IsInCheck(const Position& position, Team team);
	bool IsSquareUnderAttack(const Position& position, Team byTeam, const Square& square);
	bool IsSquareUnderAttack(const Position& position, Team byTeam, SquareIndex square);

	void ApplyMove(Position& position, const Move& move, UndoInfo* outUndoInfo = nullptr);
	void UndoMove(Position& position, const Move& move, const UndoInfo& undo);
	bool SanityCheckMove(const Position& position, const Move& move);
	Move CreateMove(const Position& position, const Square& from, const Square& to, Piece promotionPiece = PIECE_QUEEN);

	std::ostream& operator<<(std::ostream& stream, const Position& position);

}
