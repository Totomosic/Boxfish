#pragma once
#include "Bitboard.h"
#include "Position.h"
#include <limits>

namespace Boxfish
{

	using Centipawns = int;
	constexpr Centipawns SCORE_MATE = 200000;
	constexpr Centipawns SCORE_DRAW = 0;

	struct BOX_API EvaluationResult
	{
	public:
		Centipawns Material[TEAM_MAX] = { 0 };
		Centipawns Attacks[TEAM_MAX] = { 0 };
		Centipawns PieceSquares[TEAM_MAX] = { 0 };
		bool Checkmate[TEAM_MAX] = { false };
		bool Stalemate = false;
		float GameStage;

	public:
		inline bool IsCheckmate() const { return Checkmate[TEAM_WHITE] || Checkmate[TEAM_BLACK]; }

		inline Centipawns GetTotal(Team team) const
		{
			if (Checkmate[team])
				return SCORE_MATE;
			if (Checkmate[OtherTeam(team)])
				return -SCORE_MATE;
			if (Stalemate)
				return 0;
			Team other = OtherTeam(team);
			return (Material[team] - Material[other]) + (Attacks[team] - Attacks[other]) + (PieceSquares[team] - PieceSquares[other]);
		}
	};

	void InitEvaluation();

	EvaluationResult EvaluateDetailed(const Position& position);
	Centipawns Evaluate(const Position& position, Team team);

	Centipawns GetPieceValue(const Position& position, Piece piece);
	Centipawns GetPieceValue(Piece piece);

}
