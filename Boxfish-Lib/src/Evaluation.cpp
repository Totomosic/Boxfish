#include "Evaluation.h"
#include "MoveGenerator.h"

namespace Boxfish
{

	static bool s_Initialized = false;

	int s_PhaseWeights[PIECE_MAX];
	int s_MaxPhaseValue = 0;

	Centipawns s_MaterialValues[GAME_STAGE_MAX][PIECE_MAX];

	void InitPhase()
	{
		s_PhaseWeights[PIECE_PAWN] = 0;
		s_PhaseWeights[PIECE_KNIGHT] = 1;
		s_PhaseWeights[PIECE_BISHOP] = 1;
		s_PhaseWeights[PIECE_ROOK] = 2;
		s_PhaseWeights[PIECE_QUEEN] = 4;
		s_PhaseWeights[PIECE_KING] = 0;

		s_MaxPhaseValue = s_PhaseWeights[PIECE_PAWN] * 16 + s_PhaseWeights[PIECE_KNIGHT] * 4 + s_PhaseWeights[PIECE_BISHOP] * 4 + s_PhaseWeights[PIECE_ROOK] * 4 + s_PhaseWeights[PIECE_QUEEN] * 2 + s_PhaseWeights[PIECE_KING] * 2;
	}

	void InitMaterialTable()
	{
		s_MaterialValues[MIDGAME][PIECE_PAWN] = 100;
		s_MaterialValues[MIDGAME][PIECE_KNIGHT] = 320;
		s_MaterialValues[MIDGAME][PIECE_BISHOP] = 330;
		s_MaterialValues[MIDGAME][PIECE_ROOK] = 500;
		s_MaterialValues[MIDGAME][PIECE_QUEEN] = 900;
		s_MaterialValues[MIDGAME][PIECE_KING] = 0;

		s_MaterialValues[ENDGAME][PIECE_PAWN] = 100;
		s_MaterialValues[ENDGAME][PIECE_KNIGHT] = 320;
		s_MaterialValues[ENDGAME][PIECE_BISHOP] = 330;
		s_MaterialValues[ENDGAME][PIECE_ROOK] = 500;
		s_MaterialValues[ENDGAME][PIECE_QUEEN] = 900;
		s_MaterialValues[ENDGAME][PIECE_KING] = 0;
	}

	void InitEvaluation()
	{
		if (!s_Initialized)
		{
			InitPhase();
			InitMaterialTable();
			s_Initialized = true;
		}
	}

	float CalculateGameStage(const Position& position)
	{
		int phase = s_MaxPhaseValue;
		for (Piece piece = PIECE_PAWN; piece < PIECE_MAX; piece++)
		{
			BitBoard pieceBoard = position.Teams[TEAM_WHITE].Pieces[piece] | position.Teams[TEAM_BLACK].Pieces[piece];
			phase -= pieceBoard.GetCount() * s_PhaseWeights[piece];
		}
		phase = std::max(phase, 0);
		return std::min(std::max((float)(((float)phase) + (s_MaxPhaseValue / 2.0f)) / (float)s_MaxPhaseValue, 0.0f), 1.0f);
	}

	Centipawns InterpolateGameStage(float stage, Centipawns midgame, Centipawns endgame)
	{
		return (Centipawns)(midgame + (endgame - midgame) * stage);
	}

	void EvaluateMaterial(EvaluationResult& result, const Position& position, Team team, float stage)
	{
		Centipawns material = 0;
		for (Piece piece = PIECE_PAWN; piece < PIECE_MAX; piece++)
		{
			Centipawns midgame = s_MaterialValues[MIDGAME][piece];
			Centipawns endgame = s_MaterialValues[ENDGAME][piece];
			int count = position.Teams[team].Pieces[piece].GetCount();
			material += InterpolateGameStage(stage, midgame * count, endgame * count);
		}
		result.Material[team] = material;
	}

	void EvaluateMaterial(EvaluationResult& result, const Position& position, float stage)
	{
		EvaluateMaterial(result, position, TEAM_WHITE, stage);
		EvaluateMaterial(result, position, TEAM_BLACK, stage);
	}

	void EvaluateCheckmate(EvaluationResult& result, const Position& position, float stage)
	{
		MoveGenerator generator(position);
		const std::vector<Move>& legalMoves = generator.GetLegalMoves();
		if (legalMoves.empty())
		{
			if (IsInCheck(position, position.TeamToPlay))
			{
				result.Checkmate[OtherTeam(position.TeamToPlay)] = true;
			}
			else
			{
				result.Stalemate = true;
			}
		}
	}

	EvaluationResult EvaluateDetailed(const Position& position)
	{
		EvaluationResult result;
		float gameStage = CalculateGameStage(position);
		EvaluateCheckmate(result, position, gameStage);
		EvaluateMaterial(result, position, gameStage);
		return result;
	}

	Centipawns Evaluate(const Position& position, Team team)
	{
		return EvaluateDetailed(position).GetTotal(team);
	}

}
