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

	inline Centipawns InterpolateGameStage(float stage, Centipawns midgame, Centipawns endgame)
	{
		return (Centipawns)(midgame + (endgame - midgame) * stage);
	}

	struct BOX_API EvaluationResult
	{
	public:
		Centipawns Material[GAME_STAGE_MAX][TEAM_MAX];
		Centipawns Attacks[GAME_STAGE_MAX][TEAM_MAX];
		Centipawns PieceSquares[GAME_STAGE_MAX][TEAM_MAX];
		Centipawns PassedPawns[GAME_STAGE_MAX][TEAM_MAX];
		Centipawns WeakPawns[GAME_STAGE_MAX][TEAM_MAX];
		Centipawns BlockedPieces[GAME_STAGE_MAX][TEAM_MAX];
		Centipawns DoubledPawns[GAME_STAGE_MAX][TEAM_MAX];
		Centipawns KingSafety[GAME_STAGE_MAX][TEAM_MAX];
		Centipawns Tempo[GAME_STAGE_MAX][TEAM_MAX];
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
			Centipawns midgame =
				(Material[MIDGAME][team] - Material[MIDGAME][other]) +
				(Attacks[MIDGAME][team] - Attacks[MIDGAME][other]) +
				(PieceSquares[MIDGAME][team] - PieceSquares[MIDGAME][other]) +
				(PassedPawns[MIDGAME][team] - PassedPawns[MIDGAME][other]) +
				(WeakPawns[MIDGAME][team] - WeakPawns[MIDGAME][other]) +
				(BlockedPieces[MIDGAME][team] - BlockedPieces[MIDGAME][other]) +
				(DoubledPawns[MIDGAME][team] - DoubledPawns[MIDGAME][other]) +
				(KingSafety[MIDGAME][team] - KingSafety[MIDGAME][other]) +
				(Tempo[MIDGAME][team] - Tempo[MIDGAME][other]);
			Centipawns endgame = 
				(Material[ENDGAME][team] - Material[ENDGAME][other]) +
				(Attacks[ENDGAME][team] - Attacks[ENDGAME][other]) +
				(PieceSquares[ENDGAME][team] - PieceSquares[ENDGAME][other]) +
				(PassedPawns[ENDGAME][team] - PassedPawns[ENDGAME][other]) +
				(WeakPawns[ENDGAME][team] - WeakPawns[ENDGAME][other]) +
				(BlockedPieces[ENDGAME][team] - BlockedPieces[ENDGAME][other]) +
				(DoubledPawns[ENDGAME][team] - DoubledPawns[ENDGAME][other]) +
				(KingSafety[ENDGAME][team] - KingSafety[ENDGAME][other]) +
				(Tempo[ENDGAME][team] - Tempo[ENDGAME][other]);
			return InterpolateGameStage(this->GameStage, midgame, endgame);
		}
	};

	void InitEvaluation();

	EvaluationResult EvaluateDetailed(const Position& position);
	Centipawns Evaluate(const Position& position, Team team);

	Centipawns GetPieceValue(const Position& position, Piece piece);
	Centipawns GetPieceValue(Piece piece);

	std::string FormatEvaluation(const EvaluationResult& evaluation);

}
