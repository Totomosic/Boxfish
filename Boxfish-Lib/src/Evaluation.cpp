#include "Evaluation.h"
#include "MoveGenerator.h"
#include "Attacks.h"

namespace Boxfish
{

	static bool s_Initialized = false;

	static Move s_MoveBuffer[MAX_MOVES];
	static MoveList s_MoveList(nullptr, s_MoveBuffer);

	int s_PhaseWeights[PIECE_MAX];
	int s_MaxPhaseValue = 0;

	int s_DistanceTable[FILE_MAX * RANK_MAX][FILE_MAX * RANK_MAX];

	Centipawns s_MaterialValues[GAME_STAGE_MAX][PIECE_MAX];
	Centipawns s_PieceSquareTables[GAME_STAGE_MAX][TEAM_MAX][PIECE_MAX][FILE_MAX * RANK_MAX];

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
		s_MaterialValues[MIDGAME][PIECE_KING] = 20000;

		s_MaterialValues[ENDGAME][PIECE_PAWN] = 140;
		s_MaterialValues[ENDGAME][PIECE_KNIGHT] = 320;
		s_MaterialValues[ENDGAME][PIECE_BISHOP] = 330;
		s_MaterialValues[ENDGAME][PIECE_ROOK] = 500;
		s_MaterialValues[ENDGAME][PIECE_QUEEN] = 900;
		s_MaterialValues[ENDGAME][PIECE_KING] = 20000;
	}

	void MirrorTable(Centipawns* dest, Centipawns* src)
	{
		for (File file = FILE_A; file < FILE_MAX; file++)
		{
			for (Rank rank = RANK_1; rank < RANK_MAX; rank++)
			{
				SquareIndex srcIndex = BitBoard::SquareToBitIndex({ file, rank });
				SquareIndex dstIndex = BitBoard::SquareToBitIndex({ file, (Rank)(RANK_MAX - rank - 1) });
				dest[dstIndex] = src[srcIndex];
			}
		}
	}

	void InitPieceSquareTables()
	{
		Centipawns whitePawnsTable[FILE_MAX * RANK_MAX] = {
			 0,  0,  0,  0,  0,  0,  0,  0,
			50, 50, 50, 50, 50, 50, 50, 50,
			10, 10, 20, 30, 30, 20, 10, 10,
			 5,  5, 10, 25, 25, 10,  5,  5,
			 0,  0,  0, 20, 20,  0,  0,  0,
			 5, -5,-10,  0,  0,-10, -5,  5,
			 5, 10, 10,-20,-20, 10, 10,  5,
			 0,  0,  0,  0,  0,  0,  0,  0
		};
		MirrorTable(s_PieceSquareTables[MIDGAME][TEAM_WHITE][PIECE_PAWN], whitePawnsTable);
		MirrorTable(s_PieceSquareTables[MIDGAME][TEAM_BLACK][PIECE_PAWN], s_PieceSquareTables[MIDGAME][TEAM_WHITE][PIECE_PAWN]);
		MirrorTable(s_PieceSquareTables[ENDGAME][TEAM_WHITE][PIECE_PAWN], s_PieceSquareTables[MIDGAME][TEAM_BLACK][PIECE_PAWN]);
		MirrorTable(s_PieceSquareTables[ENDGAME][TEAM_BLACK][PIECE_PAWN], s_PieceSquareTables[MIDGAME][TEAM_WHITE][PIECE_PAWN]);

		Centipawns whiteKnightsTable[FILE_MAX * RANK_MAX] = {
			-50,-40,-30,-30,-30,-30,-40,-50,
			-40,-20,  0,  0,  0,  0,-20,-40,
			-30,  0, 10, 15, 15, 10,  0,-30,
			-30,  5, 15, 20, 20, 15,  5,-30,
			-30,  0, 15, 20, 20, 15,  0,-30,
			-30,  5, 10, 15, 15, 10,  5,-30,
			-40,-20,  0,  5,  5,  0,-20,-40,
			-50,-40,-30,-30,-30,-30,-40,-50,
		};
		MirrorTable(s_PieceSquareTables[MIDGAME][TEAM_WHITE][PIECE_KNIGHT], whiteKnightsTable);
		MirrorTable(s_PieceSquareTables[MIDGAME][TEAM_BLACK][PIECE_KNIGHT], s_PieceSquareTables[MIDGAME][TEAM_WHITE][PIECE_KNIGHT]);
		MirrorTable(s_PieceSquareTables[ENDGAME][TEAM_WHITE][PIECE_KNIGHT], s_PieceSquareTables[MIDGAME][TEAM_BLACK][PIECE_KNIGHT]);
		MirrorTable(s_PieceSquareTables[ENDGAME][TEAM_BLACK][PIECE_KNIGHT], s_PieceSquareTables[MIDGAME][TEAM_WHITE][PIECE_KNIGHT]);

		Centipawns whiteBishopsTable[FILE_MAX * RANK_MAX] = {
			-20,-10,-10,-10,-10,-10,-10,-20,
			-10,  0,  0,  0,  0,  0,  0,-10,
			-10,  0,  5, 10, 10,  5,  0,-10,
			-10,  5,  5, 10, 10,  5,  5,-10,
			-10,  0, 10, 10, 10, 10,  0,-10,
			-10, 10, 10, 10, 10, 10, 10,-10,
			-10,  5,  0,  0,  0,  0,  5,-10,
			-20,-10,-10,-10,-10,-10,-10,-20,
		};
		MirrorTable(s_PieceSquareTables[MIDGAME][TEAM_WHITE][PIECE_BISHOP], whiteBishopsTable);
		MirrorTable(s_PieceSquareTables[MIDGAME][TEAM_BLACK][PIECE_BISHOP], s_PieceSquareTables[MIDGAME][TEAM_WHITE][PIECE_BISHOP]);
		MirrorTable(s_PieceSquareTables[ENDGAME][TEAM_WHITE][PIECE_BISHOP], s_PieceSquareTables[MIDGAME][TEAM_BLACK][PIECE_BISHOP]);
		MirrorTable(s_PieceSquareTables[ENDGAME][TEAM_BLACK][PIECE_BISHOP], s_PieceSquareTables[MIDGAME][TEAM_WHITE][PIECE_BISHOP]);

		Centipawns whiteRooksTable[FILE_MAX * RANK_MAX] = {
			  0,  0,  0,  0,  0,  0,  0,  0,
			  5, 10, 10, 10, 10, 10, 10,  5,
			 -5,  0,  0,  0,  0,  0,  0, -5,
			 -5,  0,  0,  0,  0,  0,  0, -5,
			 -5,  0,  0,  0,  0,  0,  0, -5,
			 -5,  0,  0,  0,  0,  0,  0, -5,
			 -5,  0,  0,  0,  0,  0,  0, -5,
			  0,  0,  0,  5,  5,  0,  0,  0
		};
		MirrorTable(s_PieceSquareTables[MIDGAME][TEAM_WHITE][PIECE_ROOK], whiteRooksTable);
		MirrorTable(s_PieceSquareTables[MIDGAME][TEAM_BLACK][PIECE_ROOK], s_PieceSquareTables[MIDGAME][TEAM_WHITE][PIECE_ROOK]);
		MirrorTable(s_PieceSquareTables[ENDGAME][TEAM_WHITE][PIECE_ROOK], s_PieceSquareTables[MIDGAME][TEAM_BLACK][PIECE_ROOK]);
		MirrorTable(s_PieceSquareTables[ENDGAME][TEAM_BLACK][PIECE_ROOK], s_PieceSquareTables[MIDGAME][TEAM_WHITE][PIECE_ROOK]);

		Centipawns whiteQueensTable[FILE_MAX * RANK_MAX] = {
			-20,-10,-10, -5, -5,-10,-10,-20,
			-10,  0,  0,  0,  0,  0,  0,-10,
			-10,  0,  5,  5,  5,  5,  0,-10,
			 -5,  0,  5,  5,  5,  5,  0, -5,
			  0,  0,  5,  5,  5,  5,  0, -5,
			-10,  5,  5,  5,  5,  5,  0,-10,
			-10,  0,  5,  0,  0,  0,  0,-10,
			-20,-10,-10, -5, -5,-10,-10,-20
		};
		MirrorTable(s_PieceSquareTables[MIDGAME][TEAM_WHITE][PIECE_QUEEN], whiteQueensTable);
		MirrorTable(s_PieceSquareTables[MIDGAME][TEAM_BLACK][PIECE_QUEEN], s_PieceSquareTables[MIDGAME][TEAM_WHITE][PIECE_QUEEN]);
		MirrorTable(s_PieceSquareTables[ENDGAME][TEAM_WHITE][PIECE_QUEEN], s_PieceSquareTables[MIDGAME][TEAM_BLACK][PIECE_QUEEN]);
		MirrorTable(s_PieceSquareTables[ENDGAME][TEAM_BLACK][PIECE_QUEEN], s_PieceSquareTables[MIDGAME][TEAM_WHITE][PIECE_QUEEN]);

		Centipawns whiteKingsTableMidgame[FILE_MAX * RANK_MAX] = {
			-30,-40,-40,-50,-50,-40,-40,-30,
			-30,-40,-40,-50,-50,-40,-40,-30,
			-30,-40,-40,-50,-50,-40,-40,-30,
			-30,-40,-40,-50,-50,-40,-40,-30,
			-20,-30,-30,-40,-40,-30,-30,-20,
			-10,-20,-20,-20,-20,-20,-20,-10,
			 20, 20,  0,  0,  0,  0, 20, 20,
			 20, 30, 10,  0,  0, 10, 30, 20
		};
		MirrorTable(s_PieceSquareTables[MIDGAME][TEAM_WHITE][PIECE_KING], whiteKingsTableMidgame);
		MirrorTable(s_PieceSquareTables[MIDGAME][TEAM_BLACK][PIECE_KING], s_PieceSquareTables[MIDGAME][TEAM_WHITE][PIECE_KING]);

		Centipawns whiteKingsTableEndgame[FILE_MAX * RANK_MAX] = {
			-50,-40,-30,-20,-20,-30,-40,-50,
			-30,-20,-10,  0,  0,-10,-20,-30,
			-30,-10, 20, 30, 30, 20,-10,-30,
			-30,-10, 30, 40, 40, 30,-10,-30,
			-30,-10, 30, 40, 40, 30,-10,-30,
			-30,-10, 20, 30, 30, 20,-10,-30,
			-30,-30,  0,  0,  0,  0,-30,-30,
			-50,-30,-30,-30,-30,-30,-30,-50
		};
		MirrorTable(s_PieceSquareTables[ENDGAME][TEAM_WHITE][PIECE_KING], whiteKingsTableEndgame);
		MirrorTable(s_PieceSquareTables[ENDGAME][TEAM_BLACK][PIECE_KING], s_PieceSquareTables[ENDGAME][TEAM_WHITE][PIECE_KING]);
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
			InitPieceSquareTables();
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

	inline Centipawns InterpolateGameStage(float stage, Centipawns midgame, Centipawns endgame)
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

	void EvaluatePieceSquareTables(EvaluationResult& result, const Position& position, float stage, Team team)
	{
		Centipawns score = 0;
		for (Piece piece = PIECE_PAWN; piece < PIECE_MAX; piece++)
		{
			BitBoard pieces = position.GetTeamPieces(team, piece);
			while (pieces)
			{
				SquareIndex square = PopLeastSignificantBit(pieces);
				Centipawns mg = s_PieceSquareTables[MIDGAME][team][piece][square];
				// Only the king changes throughout the game
				if (piece == PIECE_KING)
				{
					Centipawns eg = s_PieceSquareTables[ENDGAME][team][piece][square];
					score += InterpolateGameStage(stage, mg, eg);
				}
				else
				{
					score += mg;
				}
			}
		}
		result.PieceSquares[team] = score;
	}

	void EvaluatePieceSquareTables(EvaluationResult& result, const Position& position, float stage)
	{
		EvaluatePieceSquareTables(result, position, stage, TEAM_WHITE);
		EvaluatePieceSquareTables(result, position, stage, TEAM_BLACK);
	}

	void EvaluateMobility(EvaluationResult& result, const Position& position, float stage)
	{
		
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
		Centipawns mg = count * 1;
		Centipawns eg = count * 2;
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
		if (!movegen.HasAtLeastOneLegalMove(s_MoveList))
		{
			if (IsInCheck(position, TEAM_WHITE))
			{
				result.Checkmate[TEAM_BLACK] = true;
			}
			else if (IsInCheck(position, TEAM_BLACK))
			{
				result.Checkmate[TEAM_WHITE] = true;
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
		if (!result.IsCheckmate() && !result.Stalemate)
		{
			EvaluateMaterial(result, position, gameStage);
			EvaluateMobility(result, position, gameStage);
			EvaluateAttacks(result, position, gameStage);
			EvaluatePieceSquareTables(result, position, gameStage);
		}
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

	Centipawns GetPieceValue(Piece piece)
	{
		return s_MaterialValues[MIDGAME][piece];
	}

}
