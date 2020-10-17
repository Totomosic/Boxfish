#pragma once
#include "Position.h"
#include "Move.h"
#include "Evaluation.h"
#include <string>
#include <iostream>

namespace Boxfish
{

	struct BOX_API UndoInfo
	{
	public:
		int HalfTurnsSinceCaptureOrPush;
		Square EnpassantSquare = INVALID_SQUARE;
		bool CastleKingSide[TEAM_MAX];
		bool CastleQueenSide[TEAM_MAX];
		
		BitBoard CheckedBy[TEAM_MAX];
		bool InCheck[TEAM_MAX];
	};

	Position CreateStartingPosition();
	Position CreatePositionFromFEN(const std::string& fen);
	std::string GetFENFromPosition(const Position& position);
	Position MirrorPosition(const Position& position);

	BitBoard GetAttackers(const Position& position, Team team, const Square& square, const BitBoard& blockers);
	BitBoard GetAttackers(const Position& position, Team team, SquareIndex square, const BitBoard& blockers);
	// Returns a bitboard containing all the pieces that are pinned by sliders to the square
	// 'sliders' is a bitboard containing a superset of all sliding pieces to check against
	BitBoard GetSliderBlockers(const Position& position, const BitBoard& sliders, SquareIndex square, BitBoard* pinners);

	void CalculateKingBlockers(Position& position, Team team);
	void CalculateCheckers(Position& position, Team team);

	bool IsSquareUnderAttack(const Position& position, Team byTeam, const Square& square);
	bool IsSquareUnderAttack(const Position& position, Team byTeam, SquareIndex square);

	bool SeeGE(const Position& position, const Move& move, ValueType threshold = 0);

	void ApplyMove(Position& position, Move move);
	void ApplyMove(Position& position, Move move, UndoInfo* outUndoInfo);
	void UndoMove(Position& position, Move move, const UndoInfo& undo);
	void ApplyNullMove(Position& position, UndoInfo* outUndoInfo);
	void UndoNullMove(Position& position, const UndoInfo& undo);
	bool SanityCheckMove(const Position& position, Move move);
	Move CreateMove(const Position& position, const Square& from, const Square& to, Piece promotionPiece = PIECE_QUEEN);

	std::ostream& operator<<(std::ostream& stream, const Position& position);

}
