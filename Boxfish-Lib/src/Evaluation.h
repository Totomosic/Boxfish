#pragma once
#include "Bitboard.h"
#include "Position.h"
#include <limits>

namespace Boxfish
{

	using Centipawns = int;
	constexpr Centipawns INF = 100000;

	struct BOX_API EvaluationResult
	{
	public:
		Centipawns Material[TEAM_MAX] = { 0 };
		bool Checkmate[TEAM_MAX] = { false };
		bool Stalemate = false;

	public:
		inline Centipawns GetTotal(Team team) const
		{
			if (Checkmate[team])
				return INF;
			if (Checkmate[OtherTeam(team)])
				return -INF;
			if (Stalemate)
				return 0;
			return Material[team] - Material[OtherTeam(team)];
		}
	};

	void InitEvaluation();

	EvaluationResult EvaluateDetailed(const Position& position);
	Centipawns Evaluate(const Position& position, Team team);

}
