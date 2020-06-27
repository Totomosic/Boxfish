#include "Evaluation.h"
#include "MoveGenerator.h"
#include "Attacks.h"

namespace Boxfish
{

	static bool s_Initialized = false;

	int s_PhaseWeights[PIECE_MAX];
	int s_MaxPhaseValue = 0;

	int s_DistanceTable[FILE_MAX * RANK_MAX][FILE_MAX * RANK_MAX];

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

		s_MaterialValues[ENDGAME][PIECE_PAWN] = 140;
		s_MaterialValues[ENDGAME][PIECE_KNIGHT] = 320;
		s_MaterialValues[ENDGAME][PIECE_BISHOP] = 330;
		s_MaterialValues[ENDGAME][PIECE_ROOK] = 500;
		s_MaterialValues[ENDGAME][PIECE_QUEEN] = 900;
		s_MaterialValues[ENDGAME][PIECE_KING] = 0;
	}

	void InitDistanceTable()
	{
		for (SquareIndex i = a1; i < FILE_MAX * RANK_MAX; i++)
		{
			for (SquareIndex j = a1; j < FILE_MAX * RANK_MAX; j++)
			{
				Square sqi = BitBoard::BitIndexToSquare(i);
				Square sqj = BitBoard::BitIndexToSquare(j);
				s_DistanceTable[i][j] = 14 - (std::abs(sqi.File - sqj.File) + std::abs(sqi.Rank - sqi.File));
			}
		}
	}

	void InitEvaluation()
	{
		if (!s_Initialized)
		{
			InitPhase();
			InitDistanceTable();
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
		return ((phase * 256 + (s_MaxPhaseValue / 2)) / s_MaxPhaseValue) / 256.0f;
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

	void EvaluatePieceSquareTables(EvaluationResult& result, const Position& position, float stage)
	{
		
	}

	void EvaluateMobility(EvaluationResult& result, const Position& position, float stage)
	{
		
	}

	void EvaluateBishopPairs(EvaluationResult& result, const Position& position, float stage)
	{
		constexpr Centipawns mg = 10;
		constexpr Centipawns eg = 20;
		if (position.Teams[TEAM_WHITE].Pieces[PIECE_BISHOP].GetCount() >= 2)
			result.BishopPairs[TEAM_WHITE] = InterpolateGameStage(stage, mg, eg);
		if (position.Teams[TEAM_BLACK].Pieces[PIECE_BISHOP].GetCount() >= 2)
			result.BishopPairs[TEAM_BLACK] = InterpolateGameStage(stage, mg, eg);
	}

	void EvaluateAttacks(EvaluationResult& result, const Position& position, float stage, Team team)
	{
		BitBoard attacks;
		Position board = position;

		BitBoard blockers = position.GetAllPieces();
		BitBoard& queens = board.Teams[team].Pieces[PIECE_QUEEN];
		while (queens)
		{
			SquareIndex square = PopLeastSignificantBit(queens);
			attacks |= GetSlidingAttacks(PIECE_QUEEN, square, blockers);
		}
		BitBoard& rooks = board.Teams[team].Pieces[PIECE_ROOK];
		while (rooks)
		{
			SquareIndex square = PopLeastSignificantBit(rooks);
			attacks |= GetSlidingAttacks(PIECE_ROOK, square, blockers);
		}
		BitBoard& bishops = board.Teams[team].Pieces[PIECE_BISHOP];
		while (bishops)
		{
			SquareIndex square = PopLeastSignificantBit(bishops);
			attacks |= GetSlidingAttacks(PIECE_BISHOP, square, blockers);
		}

		int count = attacks.GetCount();
		Centipawns mg = count * 3;
		Centipawns eg = count * 5;
		result.Attacks[team] = InterpolateGameStage(stage, mg, eg);
	}

	void EvaluateAttacks(EvaluationResult& result, const Position& position, float stage)
	{
		EvaluateAttacks(result, position, stage, TEAM_WHITE);
		EvaluateAttacks(result, position, stage, TEAM_BLACK);
	}

	void EvaluateCheckmate(EvaluationResult& result, const Position& position, float stage)
	{
		MoveGenerator movegen(position);
		if (!movegen.HasAtLeastOneLegalMove())
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
		result.GameStage = gameStage;
		EvaluateCheckmate(result, position, gameStage);
		EvaluateMaterial(result, position, gameStage);
		EvaluateBishopPairs(result, position, gameStage);
		EvaluateMobility(result, position, gameStage);
		EvaluateAttacks(result, position, gameStage);
		EvaluatePieceSquareTables(result, position, gameStage);
		return result;
	}

	Centipawns Evaluate(const Position& position, Team team)
	{
		return EvaluateDetailed(position).GetTotal(team);
	}

	Centipawns GetPieceValue(const Position& position, Piece piece)
	{
		float gameStage = CalculateGameStage(position);
		Centipawns mg = s_MaterialValues[MIDGAME][piece];
		Centipawns eg = s_MaterialValues[ENDGAME][piece];
		return InterpolateGameStage(gameStage, mg, eg);
	}

}
