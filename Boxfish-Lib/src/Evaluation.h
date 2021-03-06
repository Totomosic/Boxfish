#pragma once
#include "Bitboard.h"
#include "Position.h"
#include <limits>

#ifdef SWIG
#define BOX_API
#endif

namespace Boxfish
{

	extern int s_MaxPhaseValue;

	constexpr ValueType SCORE_MATE = 200000;
	constexpr ValueType SCORE_DRAW = 0;
	constexpr ValueType SCORE_NONE = -SCORE_MATE - 100;

	inline ValueType InterpolateGameStage(int stage, ValueType midgame, ValueType endgame)
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
		BitBoard AttackedBy[TEAM_MAX][PIECE_ALL + 1];
		BitBoard AttackedByTwice[TEAM_MAX];
	};

	struct BOX_API EvaluationResult
	{
	public:
		EvaluationMeta Data;

		ValueType Material[GAME_STAGE_MAX][TEAM_MAX];
		ValueType PieceSquares[GAME_STAGE_MAX][TEAM_MAX];
		ValueType BlockedPieces[GAME_STAGE_MAX][TEAM_MAX];
		ValueType Pawns[GAME_STAGE_MAX][TEAM_MAX];
		ValueType Knights[GAME_STAGE_MAX][TEAM_MAX];
		ValueType Bishops[GAME_STAGE_MAX][TEAM_MAX];
		ValueType Rooks[GAME_STAGE_MAX][TEAM_MAX];
		ValueType Queens[GAME_STAGE_MAX][TEAM_MAX];
		ValueType Space[GAME_STAGE_MAX][TEAM_MAX];
		ValueType KingSafety[GAME_STAGE_MAX][TEAM_MAX];
		ValueType Threats[GAME_STAGE_MAX][TEAM_MAX];
		ValueType Tempo[GAME_STAGE_MAX][TEAM_MAX];
		int GameStage;
		bool IsDraw;

	public:
		template<Team TEAM, Piece PIECE>
		inline void SetPieceEval(ValueType mg, ValueType eg)
		{
			if constexpr (PIECE == PIECE_KNIGHT)
			{
				Knights[MIDGAME][TEAM] = mg;
				Knights[ENDGAME][TEAM] = eg;
			}
			if constexpr (PIECE == PIECE_BISHOP)
			{
				Bishops[MIDGAME][TEAM] = mg;
				Bishops[ENDGAME][TEAM] = eg;
			}
			if constexpr (PIECE == PIECE_ROOK)
			{
				Rooks[MIDGAME][TEAM] = mg;
				Rooks[ENDGAME][TEAM] = eg;
			}
			if constexpr (PIECE == PIECE_QUEEN)
			{
				Queens[MIDGAME][TEAM] = mg;
				Queens[ENDGAME][TEAM] = eg;
			}
		}

		inline ValueType GetTotal(Team team) const
		{
			if (IsDraw)
				return SCORE_DRAW;
			Team other = OtherTeam(team);
			ValueType midgame =
				(Material[MIDGAME][team] - Material[MIDGAME][other]) +
				(PieceSquares[MIDGAME][team] - PieceSquares[MIDGAME][other]) +
				(BlockedPieces[MIDGAME][team] - BlockedPieces[MIDGAME][other]) +
				(Pawns[MIDGAME][team] - Pawns[MIDGAME][other]) +
				(Knights[MIDGAME][team] - Knights[MIDGAME][other]) +
				(Bishops[MIDGAME][team] - Bishops[MIDGAME][other]) +
				(Rooks[MIDGAME][team] - Rooks[MIDGAME][other]) +
				(Queens[MIDGAME][team] - Queens[MIDGAME][other]) +
				(Space[MIDGAME][team] - Space[MIDGAME][other]) +
				(KingSafety[MIDGAME][team] - KingSafety[MIDGAME][other]) +
				(Threats[MIDGAME][team] - Threats[MIDGAME][other]) +
				(Tempo[MIDGAME][team] - Tempo[MIDGAME][other]);
			ValueType endgame = 
				(Material[ENDGAME][team] - Material[ENDGAME][other]) +
				(PieceSquares[ENDGAME][team] - PieceSquares[ENDGAME][other]) +
				(BlockedPieces[ENDGAME][team] - BlockedPieces[ENDGAME][other]) +
				(Pawns[ENDGAME][team] - Pawns[ENDGAME][other]) +
				(Knights[ENDGAME][team] - Knights[ENDGAME][other]) +
				(Bishops[ENDGAME][team] - Bishops[ENDGAME][other]) +
				(Rooks[ENDGAME][team] - Rooks[ENDGAME][other]) +
				(Queens[ENDGAME][team] - Queens[ENDGAME][other]) +
				(Space[ENDGAME][team] - Space[ENDGAME][other]) +
				(KingSafety[ENDGAME][team] - KingSafety[ENDGAME][other]) +
				(Threats[ENDGAME][team] - Threats[ENDGAME][other]) +
				(Tempo[ENDGAME][team] - Tempo[ENDGAME][other]);
			return InterpolateGameStage(GameStage, midgame, endgame);
		}
	};

	void InitEvaluation();

	EvaluationResult EvaluateDetailed(const Position& position, Team team, ValueType alpha, ValueType beta);
	ValueType Evaluate(const Position& position, Team team, ValueType alpha, ValueType beta);
	EvaluationResult EvaluateDetailed(const Position& position);
	ValueType Evaluate(const Position& position, Team team);
	bool IsEndgame(const Position& position);

	ValueType GetPieceValue(const Position& position, Piece piece);
	ValueType GetPieceValue(Piece piece, GameStage stage = MIDGAME);

	std::string FormatEvaluation(const EvaluationResult& evaluation);

}
