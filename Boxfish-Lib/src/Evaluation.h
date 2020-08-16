#pragma once
#include "Bitboard.h"
#include "Position.h"
#include <limits>

namespace Boxfish
{

	extern int s_MaxPhaseValue;

	constexpr Centipawns SCORE_MATE = 200000;
	constexpr Centipawns SCORE_DRAW = 0;
	constexpr Centipawns SCORE_NONE = -SCORE_MATE * 2;

	inline Centipawns InterpolateGameStage(int stage, Centipawns midgame, Centipawns endgame)
	{
		int mgWeight = s_MaxPhaseValue - stage;
		return ((midgame * mgWeight) + (endgame * stage)) / s_MaxPhaseValue;
	}

	struct BOX_API EvaluationMeta
	{
	public:
		BitBoard KingAttackZone[TEAM_MAX];
		int AttackUnits[TEAM_MAX] = { 0 };
		int Attackers[TEAM_MAX] = { 0 };
		bool HasQueenAttacker[TEAM_MAX] = { false };
		BitBoard PawnAttacks[TEAM_MAX];
	};

	struct BOX_API EvaluationResult
	{
	public:
		EvaluationMeta Data;

		Centipawns Material[GAME_STAGE_MAX][TEAM_MAX];
		Centipawns PieceSquares[GAME_STAGE_MAX][TEAM_MAX];
		Centipawns PassedPawns[GAME_STAGE_MAX][TEAM_MAX];
		Centipawns WeakPawns[GAME_STAGE_MAX][TEAM_MAX];
		Centipawns DoubledPawns[GAME_STAGE_MAX][TEAM_MAX];
		Centipawns BlockedPieces[GAME_STAGE_MAX][TEAM_MAX];
		Centipawns Knights[GAME_STAGE_MAX][TEAM_MAX];
		Centipawns Bishops[GAME_STAGE_MAX][TEAM_MAX];
		Centipawns Rooks[GAME_STAGE_MAX][TEAM_MAX];
		Centipawns Queens[GAME_STAGE_MAX][TEAM_MAX];
		Centipawns Space[GAME_STAGE_MAX][TEAM_MAX];
		Centipawns KingSafety[GAME_STAGE_MAX][TEAM_MAX];
		Centipawns Tempo[GAME_STAGE_MAX][TEAM_MAX];
		int GameStage;

	public:
		inline Centipawns GetTotal(Team team) const
		{
			Team other = OtherTeam(team);
			Centipawns midgame =
				(Material[MIDGAME][team] - Material[MIDGAME][other]) +
				(PieceSquares[MIDGAME][team] - PieceSquares[MIDGAME][other]) +
				(PassedPawns[MIDGAME][team] - PassedPawns[MIDGAME][other]) +
				(WeakPawns[MIDGAME][team] - WeakPawns[MIDGAME][other]) +
				(DoubledPawns[MIDGAME][team] - DoubledPawns[MIDGAME][other]) +
				(BlockedPieces[MIDGAME][team] - BlockedPieces[MIDGAME][other]) +
				(Knights[MIDGAME][team] - Knights[MIDGAME][other]) +
				(Bishops[MIDGAME][team] - Bishops[MIDGAME][other]) +
				(Rooks[MIDGAME][team] - Rooks[MIDGAME][other]) +
				(Queens[MIDGAME][team] - Queens[MIDGAME][other]) +
				(Space[MIDGAME][team] - Space[MIDGAME][other]) +
				(KingSafety[MIDGAME][team] - KingSafety[MIDGAME][other]) +
				(Tempo[MIDGAME][team] - Tempo[MIDGAME][other]);
			Centipawns endgame = 
				(Material[ENDGAME][team] - Material[ENDGAME][other]) +
				(PieceSquares[ENDGAME][team] - PieceSquares[ENDGAME][other]) +
				(PassedPawns[ENDGAME][team] - PassedPawns[ENDGAME][other]) +
				(WeakPawns[ENDGAME][team] - WeakPawns[ENDGAME][other]) +
				(DoubledPawns[ENDGAME][team] - DoubledPawns[ENDGAME][other]) +
				(BlockedPieces[ENDGAME][team] - BlockedPieces[ENDGAME][other]) +
				(Knights[ENDGAME][team] - Knights[ENDGAME][other]) +
				(Bishops[ENDGAME][team] - Bishops[ENDGAME][other]) +
				(Rooks[ENDGAME][team] - Rooks[ENDGAME][other]) +
				(Queens[ENDGAME][team] - Queens[ENDGAME][other]) +
				(Space[ENDGAME][team] - Space[ENDGAME][other]) +
				(KingSafety[ENDGAME][team] - KingSafety[ENDGAME][other]) +
				(Tempo[ENDGAME][team] - Tempo[ENDGAME][other]);
			return InterpolateGameStage(this->GameStage, midgame, endgame);
		}
	};

	void InitEvaluation();

	EvaluationResult EvaluateDetailed(const Position& position, Team team, Centipawns alpha, Centipawns beta);
	Centipawns Evaluate(const Position& position, Team team, Centipawns alpha, Centipawns beta);
	EvaluationResult EvaluateDetailed(const Position& position);
	Centipawns Evaluate(const Position& position, Team team);
	bool IsEndgame(const Position& position);

	Centipawns GetPieceValue(const Position& position, Piece piece);
	Centipawns GetPieceValue(Piece piece);

	std::string FormatEvaluation(const EvaluationResult& evaluation);

}
