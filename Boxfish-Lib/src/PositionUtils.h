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
		PositionInfo InfoCache;
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
	Team GetTeamAt(const Position& position, SquareIndex square);

	void CalculateKingBlockers(Position& position, Team team);
	void CalculateCheckers(Position& position, Team team);
	void CalculateKingSquare(Position& position, Team team);

	bool IsSquareOccupied(const Position& position, Team team, const Square& square);
	bool IsSquareOccupied(const Position& position, Team team, SquareIndex square);
	Piece GetPieceAtSquare(const Position& position, Team team, const Square& square);
	Piece GetPieceAtSquare(const Position& position, Team team, SquareIndex square);
	bool IsPieceOnSquare(const Position& position, Team team, Piece piece, SquareIndex square);

	inline bool IsInCheck(const Position& position, Team team) { return position.InfoCache.InCheck[team]; }
	bool IsSquareUnderAttack(const Position& position, Team byTeam, const Square& square);
	bool IsSquareUnderAttack(const Position& position, Team byTeam, SquareIndex square);

	bool SeeGE(const Position& position, const Move& move, Centipawns threshold = 0);

	void ApplyMove(Position& position, const Move& move, UndoInfo* outUndoInfo = nullptr);
	void UndoMove(Position& position, const Move& move, const UndoInfo& undo);
	bool SanityCheckMove(const Position& position, const Move& move);
	Move CreateMove(const Position& position, const Square& from, const Square& to, Piece promotionPiece = PIECE_QUEEN);
	// e7d8q - Move piece from e7 -> e8 and promote to queen
	Move CreateMoveFromString(const Position& position, const std::string& uciString);

	std::ostream& operator<<(std::ostream& stream, const Position& position);

}
