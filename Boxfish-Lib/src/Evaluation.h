#pragma once
#include "Bitboard.h"
#include "Position.h"
#include <limits>

namespace Boxfish
{

	using Centipawns = int;
	constexpr Centipawns SCORE_MATE = 200000;
	constexpr Centipawns SCORE_DRAW = 0;
	constexpr Centipawns SCORE_NONE = -SCORE_MATE * 2;

	struct BOX_API EvaluationResult
	{
	public:
		Centipawns Material[TEAM_MAX];
		Centipawns Attacks[TEAM_MAX];
		Centipawns PieceSquares[TEAM_MAX];
		Centipawns PawnShield[TEAM_MAX];
		Centipawns PassedPawns[TEAM_MAX];
		Centipawns KingSafety[TEAM_MAX];
		bool Checkmate[TEAM_MAX];
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
			return (Material[team] - Material[other]) + (Attacks[team] - Attacks[other]) + (PieceSquares[team] - PieceSquares[other])
				+ (PawnShield[team] - PawnShield[other]) + (PassedPawns[team] - PassedPawns[other]) + (KingSafety[team] - KingSafety[other]);
		}
	};

	void InitEvaluation();

	EvaluationResult EvaluateDetailed(const Position& position);
	Centipawns Evaluate(const Position& position, Team team);

	Centipawns GetPieceValue(const Position& position, Piece piece);
	Centipawns GetPieceValue(Piece piece);

}
