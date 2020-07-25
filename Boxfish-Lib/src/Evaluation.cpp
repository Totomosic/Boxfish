#include "Evaluation.h"
#include "MoveGenerator.h"
#include "Attacks.h"

namespace Boxfish
{

	// =================================================================================================================================
	// INITIALIZATION
	// =================================================================================================================================

	static bool s_Initialized = false;

	static constexpr SquareIndex s_OppositeSquare[FILE_MAX * RANK_MAX] = {
		a8, b8, c8, d8, e8, f8, g8, h8,
		a7, b7, c7, d7, e7, f7, g7, h7,
		a6, b6, c6, d6, e6, f6, g6, h6,
		a5, b5, c5, d5, e5, f5, g5, h5,
		a4, b4, c4, d4, e4, f4, g4, h4,
		a3, b3, c3, d3, e3, f3, g3, h3,
		a2, b2, c2, d2, e2, f2, g2, h2,
		a1, b1, c1, d1, e1, f1, g1, h1,
	};

	static constexpr SquareIndex RelativeSquare(Team team, SquareIndex whiteSquare)
	{
		return (team == TEAM_WHITE) ? whiteSquare : s_OppositeSquare[whiteSquare];
	}

	static std::mutex m_MoveListLock;
	static Move s_MoveBuffer[MAX_MOVES];
	static MoveList s_MoveList(nullptr, s_MoveBuffer);

	static int s_PhaseWeights[PIECE_MAX];
	static int s_MaxPhaseValue = 0;

	static int s_DistanceTable[FILE_MAX * RANK_MAX][FILE_MAX * RANK_MAX];

	static Centipawns s_MaterialValues[GAME_STAGE_MAX][PIECE_MAX];
	static Centipawns s_PieceSquareTables[GAME_STAGE_MAX][TEAM_MAX][PIECE_MAX][FILE_MAX * RANK_MAX];

	static BitBoard s_PawnShieldMasks[TEAM_MAX][FILE_MAX * RANK_MAX];
	static BitBoard s_PassedPawnMasks[TEAM_MAX][FILE_MAX * RANK_MAX];
	static Centipawns s_PassedPawnRankWeights[GAME_STAGE_MAX][TEAM_MAX][RANK_MAX];
	static BitBoard s_SupportedPawnMasks[TEAM_MAX][FILE_MAX * RANK_MAX];

	// Indexed by attack units
	static constexpr int MAX_ATTACK_UNITS = 100;
	static constexpr Centipawns s_KingSafetyTable[MAX_ATTACK_UNITS] = {
		0,   0,   1,   2,   3,   5,   7,   9,  12,  15,
		18,  22,  26,  30,  35,  39,  44,  50,  56,  62,
		68,  75,  82,  85,  89,  97, 105, 113, 122, 131,
		140, 150, 169, 180, 191, 202, 213, 225, 237, 248,
		260, 272, 283, 295, 307, 319, 330, 342, 354, 366,
		377, 389, 401, 412, 424, 436, 448, 459, 471, 483,
		494, 500, 500, 500, 500, 500, 500, 500, 500, 500,
		500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
		500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
		500, 500, 500, 500, 500, 500, 500, 500, 500, 500
	};

	static int s_AttackUnits[PIECE_MAX];

	static const BitBoard s_NeighbourFiles[FILE_MAX] = {
					  FILE_B_MASK,
		FILE_A_MASK | FILE_C_MASK,
		FILE_B_MASK | FILE_D_MASK,
		FILE_C_MASK | FILE_E_MASK,
		FILE_D_MASK | FILE_F_MASK,
		FILE_E_MASK | FILE_G_MASK,
		FILE_F_MASK | FILE_H_MASK,
		FILE_G_MASK,
	};

	static const BitBoard s_NeighbourRanks[RANK_MAX] = {
					  RANK_2_MASK,
		RANK_1_MASK | RANK_3_MASK,
		RANK_2_MASK | RANK_4_MASK,
		RANK_3_MASK | RANK_5_MASK,
		RANK_4_MASK | RANK_6_MASK,
		RANK_5_MASK | RANK_7_MASK,
		RANK_6_MASK | RANK_8_MASK,
		RANK_7_MASK,
	};

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

	void InitPawnShieldMasks()
	{
		for (SquareIndex sq = a1; sq < FILE_MAX * RANK_MAX; sq++)
		{
			BitBoard square(sq);
			s_PawnShieldMasks[TEAM_WHITE][sq] = ((square << 8) | ((square << 7) & ~FILE_H_MASK) | ((square << 9) & ~FILE_A_MASK)) & RANK_2_MASK;
			s_PawnShieldMasks[TEAM_BLACK][sq] = ((square >> 8) | ((square >> 7) & ~FILE_H_MASK) | ((square >> 9) & ~FILE_A_MASK)) & RANK_7_MASK;
		}
	}

	void InitPassedPawnMasks()
	{
		for (SquareIndex sq = a1; sq < FILE_MAX * RANK_MAX; sq++)
		{
			BitBoard north = GetRay(RAY_NORTH, sq);
			BitBoard south = GetRay(RAY_SOUTH, sq);
			
			s_PassedPawnMasks[TEAM_WHITE][sq] = north | ShiftEast(north, 1) | ShiftWest(north, 1);
			s_PassedPawnMasks[TEAM_BLACK][sq] = south | ShiftWest(south, 1) | ShiftEast(south, 1);

			BitBoard square = sq;
			s_SupportedPawnMasks[TEAM_WHITE][sq] = ((square << 1) | (square >> 1) | (square >> 9) | (square >> 7)) & (~FILE_A_MASK & ~FILE_H_MASK & ~RANK_8_MASK);
			s_SupportedPawnMasks[TEAM_BLACK][sq] = ((square << 1) | (square >> 1) | (square << 9) | (square << 7)) & (~FILE_A_MASK & ~FILE_H_MASK & ~RANK_8_MASK);
		}

		s_PassedPawnRankWeights[MIDGAME][TEAM_WHITE][RANK_1] = 0;
		s_PassedPawnRankWeights[ENDGAME][TEAM_WHITE][RANK_1] = 0;
		s_PassedPawnRankWeights[MIDGAME][TEAM_BLACK][RANK_1] = 0;
		s_PassedPawnRankWeights[ENDGAME][TEAM_BLACK][RANK_1] = 0;

		s_PassedPawnRankWeights[MIDGAME][TEAM_WHITE][RANK_2] = 0;
		s_PassedPawnRankWeights[ENDGAME][TEAM_WHITE][RANK_2] = 0;
		s_PassedPawnRankWeights[MIDGAME][TEAM_BLACK][RANK_2] = 100;
		s_PassedPawnRankWeights[ENDGAME][TEAM_BLACK][RANK_2] = 200;

		s_PassedPawnRankWeights[MIDGAME][TEAM_WHITE][RANK_3] = 10;
		s_PassedPawnRankWeights[ENDGAME][TEAM_WHITE][RANK_3] = 20;
		s_PassedPawnRankWeights[MIDGAME][TEAM_BLACK][RANK_3] = 90;
		s_PassedPawnRankWeights[ENDGAME][TEAM_BLACK][RANK_3] = 180;

		s_PassedPawnRankWeights[MIDGAME][TEAM_WHITE][RANK_4] = 5;
		s_PassedPawnRankWeights[ENDGAME][TEAM_WHITE][RANK_4] = 40;
		s_PassedPawnRankWeights[MIDGAME][TEAM_BLACK][RANK_4] = 10;
		s_PassedPawnRankWeights[ENDGAME][TEAM_BLACK][RANK_4] = 80;

		s_PassedPawnRankWeights[MIDGAME][TEAM_WHITE][RANK_5] = 10;
		s_PassedPawnRankWeights[ENDGAME][TEAM_WHITE][RANK_5] = 80;
		s_PassedPawnRankWeights[MIDGAME][TEAM_BLACK][RANK_5] = 5;
		s_PassedPawnRankWeights[ENDGAME][TEAM_BLACK][RANK_5] = 40;

		s_PassedPawnRankWeights[MIDGAME][TEAM_WHITE][RANK_6] = 90;
		s_PassedPawnRankWeights[ENDGAME][TEAM_WHITE][RANK_6] = 180;
		s_PassedPawnRankWeights[MIDGAME][TEAM_BLACK][RANK_6] = 10;
		s_PassedPawnRankWeights[ENDGAME][TEAM_BLACK][RANK_6] = 20;

		s_PassedPawnRankWeights[MIDGAME][TEAM_WHITE][RANK_7] = 100;
		s_PassedPawnRankWeights[ENDGAME][TEAM_WHITE][RANK_7] = 200;
		s_PassedPawnRankWeights[MIDGAME][TEAM_BLACK][RANK_7] = 0;
		s_PassedPawnRankWeights[ENDGAME][TEAM_BLACK][RANK_7] = 0;

		s_PassedPawnRankWeights[MIDGAME][TEAM_WHITE][RANK_8] = 0;
		s_PassedPawnRankWeights[ENDGAME][TEAM_WHITE][RANK_8] = 0;
		s_PassedPawnRankWeights[MIDGAME][TEAM_BLACK][RANK_8] = 0;
		s_PassedPawnRankWeights[ENDGAME][TEAM_BLACK][RANK_8] = 0;
	}

	void InitMaterialTable()
	{
		s_MaterialValues[MIDGAME][PIECE_PAWN] = 100;
		s_MaterialValues[MIDGAME][PIECE_KNIGHT] = 320;
		s_MaterialValues[MIDGAME][PIECE_BISHOP] = 330;
		s_MaterialValues[MIDGAME][PIECE_ROOK] = 500;
		s_MaterialValues[MIDGAME][PIECE_QUEEN] = 950;
		s_MaterialValues[MIDGAME][PIECE_KING] = 20000;

		s_MaterialValues[ENDGAME][PIECE_PAWN] = 140;
		s_MaterialValues[ENDGAME][PIECE_KNIGHT] = 320;
		s_MaterialValues[ENDGAME][PIECE_BISHOP] = 330;
		s_MaterialValues[ENDGAME][PIECE_ROOK] = 500;
		s_MaterialValues[ENDGAME][PIECE_QUEEN] = 900;
		s_MaterialValues[ENDGAME][PIECE_KING] = 20000;
	}

	void InitPieceSquareTables()
	{
		Centipawns whitePawnsTable[FILE_MAX * RANK_MAX] = {
			 0,  0,  0,  0,  0,  0,  0,  0,
			50, 50, 50, 50, 50, 50, 50, 50,
			10, 10, 20, 30, 30, 20, 10, 10,
			 5,  5, 10, 25, 25, 10,  5,  5,
			 0,  0,  0, 20, 20,  0,  0,  0,
			 5, -5,-10,  5,  5,-10, -5,  5,
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

	void InitSafetyTables()
	{
		s_AttackUnits[PIECE_PAWN] = 0;
		s_AttackUnits[PIECE_KNIGHT] = 2;
		s_AttackUnits[PIECE_BISHOP] = 2;
		s_AttackUnits[PIECE_ROOK] = 3;
		s_AttackUnits[PIECE_QUEEN] = 5;
		s_AttackUnits[PIECE_KING] = 0;
	}

	void InitEvaluation()
	{
		if (!s_Initialized)
		{
			InitPhase();
			InitDistanceTable();
			InitSafetyTables();
			InitMaterialTable();
			InitPieceSquareTables();
			InitPawnShieldMasks();
			InitPassedPawnMasks();
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

	template<Team team>
	BitBoard CalculateKingAttackRegion(const Position& position)
	{
		/*BitBoard kingSquare = position.InfoCache.KingSquare[team];
		BitBoard reachableSquares = MoveGenerator::GetReachableKingSquares(position, team) | kingSquare;
		BitBoard region = reachableSquares;
		while (reachableSquares)
		{
			SquareIndex square = PopLeastSignificantBit(reachableSquares);
			Rank rank = BitBoard::BitIndexToSquare(square).Rank;
			if (team == TEAM_WHITE)
			{
				BitBoard ray = GetRay(RAY_NORTH, square) | BitBoard{ square };
				BitBoard mask = RANK_MASKS[rank];
				for (int i = 1; i <= 3 && (rank + i) < RANK_MAX; i++)
				{
					mask |= RANK_MASKS[rank + i];
				}
				region |= (ray & mask);
			}
			else
			{
				BitBoard ray = GetRay(RAY_SOUTH, square) | BitBoard{ square };
				BitBoard mask = RANK_MASKS[rank];
				for (int i = 1; i <= 3 && (rank - i) >= RANK_1; i++)
				{
					mask |= RANK_MASKS[rank - i];
				}
				region |= (ray & mask);
			}
		}
		return region;*/

		SquareIndex kingSquare = position.InfoCache.KingSquare[team];
		Square sq = BitBoard::BitIndexToSquare(kingSquare);

		BitBoard files = s_NeighbourFiles[sq.File] | FILE_MASKS[sq.File];
		if (team == TEAM_WHITE)
		{
			BitBoard ranks = RANK_MASKS[sq.Rank];
			for (int i = 1; i <= 2 && (sq.Rank + i) < RANK_MAX; i++)
			{
				ranks |= RANK_MASKS[sq.Rank + i];
			}
			if (sq.Rank > RANK_1)
			{
				ranks |= RANK_MASKS[sq.Rank - 1];
			}
			files &= ranks;
		}
		else
		{
			BitBoard ranks = RANK_MASKS[sq.Rank];
			for (int i = 1; i <= 2 && (sq.Rank - i) >= RANK_1; i++)
			{
				ranks |= RANK_MASKS[sq.Rank - i];
			}
			if (sq.Rank < RANK_8)
			{
				ranks |= RANK_MASKS[sq.Rank + 1];
			}
			files &= ranks;
		}
		return files;
	}

	// =================================================================================================================================
	// HELPER FUNCTIONS
	// =================================================================================================================================

	bool IsPawnSupported(const Position& position, SquareIndex square, Team team)
	{
		const BitBoard& pawns = position.GetTeamPieces(team, PIECE_PAWN);
		return (s_SupportedPawnMasks[team][square] & pawns) != ZERO_BB;
	}

	// =================================================================================================================================
	// EVALUATION FUNCTIONS
	// =================================================================================================================================

	void EvaluateCheckmate(EvaluationResult& result, const Position& position, float stage)
	{
		MoveGenerator movegen(position);
		result.Checkmate[TEAM_WHITE] = false;
		result.Checkmate[TEAM_BLACK] = false;
		result.Stalemate = false;
		std::scoped_lock<std::mutex> lock(m_MoveListLock);
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

	template<Team team>
	void EvaluateMaterial(EvaluationResult& result, const Position& position, float stage)
	{
		Centipawns mg = 0;
		Centipawns eg = 0;
		// Don't evaluate material of king pieces
		for (Piece piece = PIECE_PAWN; piece < PIECE_KING; piece++)
		{
			Centipawns midgame = s_MaterialValues[MIDGAME][piece];
			Centipawns endgame = s_MaterialValues[ENDGAME][piece];
			int count = position.GetTeamPieces(team, piece).GetCount();
			mg += midgame * count;
			eg += endgame * count;
		}
		result.Material[MIDGAME][team] = mg;
		result.Material[ENDGAME][team] = eg;
	}

	void EvaluateMaterial(EvaluationResult& result, const Position& position, float stage)
	{
		EvaluateMaterial<TEAM_WHITE>(result, position, stage);
		EvaluateMaterial<TEAM_BLACK>(result, position, stage);
	}

	template<Team team>
	void EvaluateAttacks(EvaluationResult& result, const Position& position, float stage)
	{
		BitBoard attacks = ZERO_BB;

		BitBoard blockers = position.GetAllPieces();
		BitBoard notTeamPieces = ~position.GetTeamPieces(team);
		BitBoard bishops = position.GetTeamPieces(team, PIECE_BISHOP);
		while (bishops)
		{
			SquareIndex square = PopLeastSignificantBit(bishops);
			attacks |= GetSlidingAttacks(PIECE_BISHOP, square, blockers) & notTeamPieces;
		}

		int count = attacks.GetCount();
		Centipawns mg = count * 2;
		Centipawns eg = count * 3;
		result.Attacks[MIDGAME][team] = mg;
		result.Attacks[ENDGAME][team] = eg;
	}

	void EvaluateAttacks(EvaluationResult& result, const Position& position, float stage)
	{
		EvaluateAttacks<TEAM_WHITE>(result, position, stage);
		EvaluateAttacks<TEAM_BLACK>(result, position, stage);
	}

	template<Team team>
	void EvaluatePieceSquareTables(EvaluationResult& result, const Position& position, float stage)
	{
		Centipawns mg = 0;
		Centipawns eg = 0;
		for (Piece piece = PIECE_PAWN; piece < PIECE_MAX; piece++)
		{
			BitBoard pieces = position.GetTeamPieces(team, piece);
			while (pieces)
			{
				SquareIndex square = PopLeastSignificantBit(pieces);
				mg += s_PieceSquareTables[MIDGAME][team][piece][square];
				eg += s_PieceSquareTables[ENDGAME][team][piece][square];
			}
		}
		result.PieceSquares[MIDGAME][team] = mg;
		result.PieceSquares[ENDGAME][team] = eg;
	}

	void EvaluatePieceSquareTables(EvaluationResult& result, const Position& position, float stage)
	{
		EvaluatePieceSquareTables<TEAM_WHITE>(result, position, stage);
		EvaluatePieceSquareTables<TEAM_BLACK>(result, position, stage);
	}

	template<Team team>
	void EvaluatePassedPawns(EvaluationResult& result, const Position& position, float stage)
	{
		Centipawns mgValue = 0;
		Centipawns egValue = 0;
		BitBoard pawns = position.GetTeamPieces(team, PIECE_PAWN);
		constexpr Team otherTeam = OtherTeam(team);

		BitBoard otherPawns = position.GetTeamPieces(otherTeam, PIECE_PAWN);

		constexpr Centipawns supportedBonus = 50;

		while (pawns)
		{
			SquareIndex square = PopLeastSignificantBit(pawns);
			if ((otherPawns & s_PassedPawnMasks[team][square]) == ZERO_BB)
			{
				Rank rank = BitBoard::BitIndexToSquare(square).Rank;
				mgValue += s_PassedPawnRankWeights[MIDGAME][team][rank];
				egValue += s_PassedPawnRankWeights[ENDGAME][team][rank];
				if (IsPawnSupported(position, square, team))
				{
					mgValue += supportedBonus;
					egValue += supportedBonus;
				}
			}
		}
		result.PassedPawns[MIDGAME][team] = mgValue;
		result.PassedPawns[ENDGAME][team] = egValue;
	}

	void EvaluatePassedPawns(EvaluationResult& result, const Position& position, float stage)
	{
		EvaluatePassedPawns<TEAM_WHITE>(result, position, stage);
		EvaluatePassedPawns<TEAM_BLACK>(result, position, stage);
	}

	template<Team team>
	void EvaluateDoubledPawns(EvaluationResult& result, const Position& position, float stage)
	{
		int count = 0;
		for (File file = FILE_A; file < FILE_MAX; file++)
		{
			int pawnsOnFile = (position.GetTeamPieces(team, PIECE_PAWN) & FILE_MASKS[file]).GetCount();
			if (pawnsOnFile > 0)
				count += pawnsOnFile - 1;
		}
		Centipawns mg = -10 * count;
		Centipawns eg = -30 * count;
		result.DoubledPawns[MIDGAME][team] = mg;
		result.DoubledPawns[ENDGAME][team] = eg;
	}

	void EvaluateDoubledPawns(EvaluationResult& result, const Position& position, float stage)
	{
		EvaluateDoubledPawns<TEAM_WHITE>(result, position, stage);
		EvaluateDoubledPawns<TEAM_BLACK>(result, position, stage);
	}

	template<Team team>
	void EvaluateWeakPawns(EvaluationResult& result, const Position& position, float stage)
	{
		int count = 0;
		BitBoard pawns = position.GetTeamPieces(team, PIECE_PAWN);
		while (pawns)
		{
			SquareIndex square = PopLeastSignificantBit(pawns);
			if (!IsPawnSupported(position, square, team))
				count++;
		}
		result.WeakPawns[MIDGAME][team] = -count * 10;
		result.WeakPawns[ENDGAME][team] = -count * 30;
	}

	void EvaluateWeakPawns(EvaluationResult& result, const Position& position, float stage)
	{
		EvaluateWeakPawns<TEAM_WHITE>(result, position, stage);
		EvaluateWeakPawns<TEAM_BLACK>(result, position, stage);
	}

	template<Team team>
	void EvaluateBlockedPieces(EvaluationResult& result, const Position& position, float stage)
	{
		constexpr Team otherTeam = OtherTeam(team);
		Centipawns mg = 0;
		Centipawns eg = 0;

		constexpr Centipawns TRAPPED_BISHOP_A8[GAME_STAGE_MAX] = { -150, -50 };
		constexpr Centipawns TRAPPED_BISHOP_A7[GAME_STAGE_MAX] = { -100, -50 };
		constexpr Centipawns TRAPPED_BISHOP_A6[GAME_STAGE_MAX] = { -50, -50 };
		constexpr Centipawns TRAPPED_BISHOP_B8[GAME_STAGE_MAX] = { -150, -50 };

		constexpr Centipawns TRAPPED_ROOK[GAME_STAGE_MAX] = { -50, -50 };

		// Trapped Bishops
		if (IsPieceOnSquare(position, team, PIECE_BISHOP, RelativeSquare(team, a7)) && IsPieceOnSquare(position, otherTeam, PIECE_PAWN, RelativeSquare(team, b6)))
		{
			mg += TRAPPED_BISHOP_A7[MIDGAME];
			eg += TRAPPED_BISHOP_A7[ENDGAME];
		}
		if (IsPieceOnSquare(position, team, PIECE_BISHOP, RelativeSquare(team, h7)) && IsPieceOnSquare(position, otherTeam, PIECE_PAWN, RelativeSquare(team, g6)))
		{
			mg += TRAPPED_BISHOP_A7[MIDGAME];
			eg += TRAPPED_BISHOP_A7[ENDGAME];
		}
		if (IsPieceOnSquare(position, team, PIECE_BISHOP, RelativeSquare(team, a6)) && IsPieceOnSquare(position, otherTeam, PIECE_PAWN, RelativeSquare(team, b5)))
		{
			mg += TRAPPED_BISHOP_A6[MIDGAME];
			eg += TRAPPED_BISHOP_A6[ENDGAME];
		}
		if (IsPieceOnSquare(position, team, PIECE_BISHOP, RelativeSquare(team, h6)) && IsPieceOnSquare(position, otherTeam, PIECE_PAWN, RelativeSquare(team, g5)))
		{
			mg += TRAPPED_BISHOP_A6[MIDGAME];
			eg += TRAPPED_BISHOP_A6[ENDGAME];
		}
		if (IsPieceOnSquare(position, team, PIECE_BISHOP, RelativeSquare(team, b8)) && IsPieceOnSquare(position, otherTeam, PIECE_PAWN, RelativeSquare(team, c7)))
		{
			mg += TRAPPED_BISHOP_B8[MIDGAME];
			eg += TRAPPED_BISHOP_B8[ENDGAME];
		}
		if (IsPieceOnSquare(position, team, PIECE_BISHOP, RelativeSquare(team, g8)) && IsPieceOnSquare(position, otherTeam, PIECE_PAWN, RelativeSquare(team, f7)))
		{
			mg += TRAPPED_BISHOP_B8[MIDGAME];
			eg += TRAPPED_BISHOP_B8[ENDGAME];
		}
		if (IsPieceOnSquare(position, team, PIECE_BISHOP, RelativeSquare(team, a8)) && IsPieceOnSquare(position, otherTeam, PIECE_PAWN, RelativeSquare(team, b7)))
		{
			mg += TRAPPED_BISHOP_A8[MIDGAME];
			eg += TRAPPED_BISHOP_A8[ENDGAME];
		}
		if (IsPieceOnSquare(position, team, PIECE_BISHOP, RelativeSquare(team, h8)) && IsPieceOnSquare(position, otherTeam, PIECE_PAWN, RelativeSquare(team, g7)))
		{
			mg += TRAPPED_BISHOP_A8[MIDGAME];
			eg += TRAPPED_BISHOP_A8[ENDGAME];
		}

		// Blocked Rook
		if ((IsPieceOnSquare(position, team, PIECE_ROOK, RelativeSquare(team, g1)) || IsPieceOnSquare(position, team, PIECE_ROOK, RelativeSquare(team, h1)))
			&& (IsPieceOnSquare(position, team, PIECE_KING, RelativeSquare(team, g1)) || IsPieceOnSquare(position, team, PIECE_KING, RelativeSquare(team, f1))))
		{
			mg += TRAPPED_ROOK[MIDGAME];
			eg += TRAPPED_ROOK[ENDGAME];
		}
		else if ((IsPieceOnSquare(position, team, PIECE_ROOK, RelativeSquare(team, a1)) || IsPieceOnSquare(position, team, PIECE_ROOK, RelativeSquare(team, b1)))
			&& (IsPieceOnSquare(position, team, PIECE_KING, RelativeSquare(team, b1)) || IsPieceOnSquare(position, team, PIECE_KING, RelativeSquare(team, c1))))
		{
			mg += TRAPPED_ROOK[MIDGAME];
			eg += TRAPPED_ROOK[ENDGAME];
		}

		result.BlockedPieces[MIDGAME][team] = mg;
		result.BlockedPieces[ENDGAME][team] = eg;
	}

	void EvaluateBlockedPieces(EvaluationResult& result, const Position& position, float stage)
	{
		EvaluateBlockedPieces<TEAM_WHITE>(result, position, stage);
		EvaluateBlockedPieces<TEAM_BLACK>(result, position, stage);
	}

	template<Team team>
	void EvaluateKingSafety(EvaluationResult& result, const Position& position, float stage)
	{
		constexpr Team otherTeam = OtherTeam(team);
		Square kingSquare = BitBoard::BitIndexToSquare(position.InfoCache.KingSquare[team]);
		BitBoard kingZone = CalculateKingAttackRegion<team>(position);
		
		BitBoard blockers = position.GetAllPieces();
		BitBoard notOtherTeamPieces = ~position.GetTeamPieces(otherTeam);
		int attackUnits = 0;
		int attackers = 0;
		for (Piece piece : { PIECE_BISHOP, PIECE_ROOK, PIECE_QUEEN })
		{
			BitBoard squares = position.GetTeamPieces(otherTeam, piece);
			while (squares)
			{
				SquareIndex square = PopLeastSignificantBit(squares);
				BitBoard attacks = GetSlidingAttacks(piece, square, blockers) & notOtherTeamPieces & kingZone;
				if (attacks)
				{
					attackUnits += s_AttackUnits[piece] * attacks.GetCount();
					attackers++;
				}
			}
		}
		BitBoard knights = position.GetTeamPieces(otherTeam, PIECE_KNIGHT);
		while (knights)
		{
			SquareIndex square = PopLeastSignificantBit(knights);
			BitBoard attacks = GetNonSlidingAttacks(PIECE_KNIGHT, square, otherTeam) & notOtherTeamPieces & kingZone;
			if (attacks)
			{
				attackUnits += s_AttackUnits[PIECE_KNIGHT] * attacks.GetCount();
				attackers++;
			}
		}

		Centipawns value = 0;
		if (attackers > 2)
		{
			value = -s_KingSafetyTable[std::min(attackUnits, MAX_ATTACK_UNITS - 1)];
		}

		constexpr Centipawns PAWN_SHIELD_1 = 15;
		constexpr Centipawns PAWN_SHIELD_2 = 10;
		if (team == TEAM_WHITE)
		{
			if (kingSquare.Rank < RANK_8)
			{
				value += (position.GetTeamPieces(team, PIECE_PAWN) & (kingZone & RANK_MASKS[kingSquare.Rank + 1])).GetCount() * PAWN_SHIELD_1;
			}
			if (kingSquare.Rank < RANK_7)
			{
				value += (position.GetTeamPieces(team, PIECE_PAWN) & (kingZone & RANK_MASKS[kingSquare.Rank + 2])).GetCount() * PAWN_SHIELD_2;
			}
		}
		else
		{
			if (kingSquare.Rank > RANK_1)
			{
				value += (position.GetTeamPieces(team, PIECE_PAWN) & (kingZone & RANK_MASKS[kingSquare.Rank - 1])).GetCount() * PAWN_SHIELD_1;
			}
			if (kingSquare.Rank > RANK_2)
			{
				value += (position.GetTeamPieces(team, PIECE_PAWN) & (kingZone & RANK_MASKS[kingSquare.Rank - 2])).GetCount() * PAWN_SHIELD_2;
			}
		}

		result.KingSafety[MIDGAME][team] = value;
		result.KingSafety[ENDGAME][team] = value / 2;
	}

	void EvaluateKingSafety(EvaluationResult& result, const Position& position, float stage)
	{
		EvaluateKingSafety<TEAM_WHITE>(result, position, stage);
		EvaluateKingSafety<TEAM_BLACK>(result, position, stage);
	}

	void EvaluateTempo(EvaluationResult& result, const Position& position, float stage)
	{
		constexpr Centipawns tempo = 10;
		Team other = OtherTeam(position.TeamToPlay);
		result.Tempo[MIDGAME][position.TeamToPlay] = tempo;
		result.Tempo[ENDGAME][position.TeamToPlay] = tempo;
		result.Tempo[MIDGAME][other] = 0;
		result.Tempo[ENDGAME][other] = 0;
	}

	// =================================================================================================================================
	// ENTRY POINTS
	// =================================================================================================================================

	EvaluationResult EvaluateDetailed(const Position& position)
	{
		EvaluationResult result;
		float gameStage = CalculateGameStage(position);
		result.GameStage = gameStage;
		EvaluateCheckmate(result, position, gameStage);
		if (!result.IsCheckmate() && !result.Stalemate)
		{
			EvaluateMaterial(result, position, gameStage);
			EvaluateAttacks(result, position, gameStage);
			EvaluatePieceSquareTables(result, position, gameStage);
			EvaluatePassedPawns(result, position, gameStage);
			EvaluateDoubledPawns(result, position, gameStage);
			EvaluateWeakPawns(result, position, gameStage);
			EvaluateBlockedPieces(result, position, gameStage);
			EvaluateKingSafety(result, position, gameStage);
			EvaluateTempo(result, position, gameStage);
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

	// =================================================================================================================================
	//  FORMATTING
	// =================================================================================================================================

	std::string FormatScore(Centipawns score, int maxPlaces)
	{
		int places = 1;
		if (std::abs(score) > 0)
			places = (int)std::log10(std::abs(score)) + 1;
		if (score < 0)
			places++;
		int spaces = maxPlaces - places;
		int left = spaces / 2;
		int right = spaces / 2;
		if (spaces % 2 == 1)
			left++;

		std::string result = "";
		for (int i = 0; i < left; i++)
			result += ' ';
		result += std::to_string(score);
		for (int i = 0; i < right; i++)
			result += ' ';
		return result;
	}

	std::string FormatEvaluation(const EvaluationResult& evaluation)
	{
		int scoreLength = 6;

		std::string whiteMidgameTotal = FormatScore(
			evaluation.Material[MIDGAME][TEAM_WHITE] +
			evaluation.Attacks[MIDGAME][TEAM_WHITE] +
			evaluation.PieceSquares[MIDGAME][TEAM_WHITE] +
			evaluation.BlockedPieces[MIDGAME][TEAM_WHITE] +
			evaluation.DoubledPawns[MIDGAME][TEAM_WHITE] +
			evaluation.WeakPawns[MIDGAME][TEAM_WHITE] +
			evaluation.PassedPawns[MIDGAME][TEAM_WHITE] +
			evaluation.KingSafety[MIDGAME][TEAM_WHITE] +
			evaluation.Tempo[MIDGAME][TEAM_WHITE],
			scoreLength
		);

		std::string whiteEndgameTotal = FormatScore(
			evaluation.Material[ENDGAME][TEAM_WHITE] +
			evaluation.Attacks[ENDGAME][TEAM_WHITE] +
			evaluation.PieceSquares[ENDGAME][TEAM_WHITE] +
			evaluation.BlockedPieces[ENDGAME][TEAM_WHITE] +
			evaluation.DoubledPawns[ENDGAME][TEAM_WHITE] +
			evaluation.WeakPawns[ENDGAME][TEAM_WHITE] +
			evaluation.PassedPawns[ENDGAME][TEAM_WHITE] +
			evaluation.KingSafety[ENDGAME][TEAM_WHITE] +
			evaluation.Tempo[ENDGAME][TEAM_WHITE],
			scoreLength
		);

		std::string blackMidgameTotal = FormatScore(
			evaluation.Material[MIDGAME][TEAM_BLACK] +
			evaluation.Attacks[MIDGAME][TEAM_BLACK] +
			evaluation.PieceSquares[MIDGAME][TEAM_BLACK] +
			evaluation.BlockedPieces[MIDGAME][TEAM_BLACK] +
			evaluation.DoubledPawns[MIDGAME][TEAM_BLACK] +
			evaluation.WeakPawns[MIDGAME][TEAM_BLACK] +
			evaluation.PassedPawns[MIDGAME][TEAM_BLACK] +
			evaluation.KingSafety[MIDGAME][TEAM_BLACK] +
			evaluation.Tempo[MIDGAME][TEAM_BLACK],
			scoreLength
		);

		std::string blackEndgameTotal = FormatScore(
			evaluation.Material[ENDGAME][TEAM_BLACK] +
			evaluation.Attacks[ENDGAME][TEAM_BLACK] +
			evaluation.PieceSquares[ENDGAME][TEAM_BLACK] +
			evaluation.BlockedPieces[ENDGAME][TEAM_BLACK] +
			evaluation.DoubledPawns[ENDGAME][TEAM_BLACK] +
			evaluation.WeakPawns[ENDGAME][TEAM_BLACK] +
			evaluation.PassedPawns[ENDGAME][TEAM_BLACK] +
			evaluation.KingSafety[ENDGAME][TEAM_BLACK] +
			evaluation.Tempo[ENDGAME][TEAM_BLACK],
			scoreLength
		);

		std::string midgameTotal = FormatScore(
			evaluation.Material[MIDGAME][TEAM_WHITE] - evaluation.Material[MIDGAME][TEAM_BLACK] +
			evaluation.Attacks[MIDGAME][TEAM_WHITE] - evaluation.Attacks[MIDGAME][TEAM_BLACK] +
			evaluation.PieceSquares[MIDGAME][TEAM_WHITE] - evaluation.PieceSquares[MIDGAME][TEAM_BLACK] +
			evaluation.BlockedPieces[MIDGAME][TEAM_WHITE] - evaluation.BlockedPieces[MIDGAME][TEAM_BLACK] +
			evaluation.DoubledPawns[MIDGAME][TEAM_WHITE] - evaluation.DoubledPawns[MIDGAME][TEAM_BLACK] +
			evaluation.WeakPawns[MIDGAME][TEAM_WHITE] - evaluation.WeakPawns[MIDGAME][TEAM_BLACK] +
			evaluation.PassedPawns[MIDGAME][TEAM_WHITE] - evaluation.PassedPawns[MIDGAME][TEAM_BLACK] +
			evaluation.KingSafety[MIDGAME][TEAM_WHITE] - evaluation.KingSafety[MIDGAME][TEAM_BLACK] +
			evaluation.Tempo[MIDGAME][TEAM_WHITE] - evaluation.Tempo[MIDGAME][TEAM_BLACK],
			scoreLength
		);

		std::string endgameTotal = FormatScore(
			evaluation.Material[ENDGAME][TEAM_WHITE] - evaluation.Material[ENDGAME][TEAM_BLACK] +
			evaluation.Attacks[ENDGAME][TEAM_WHITE] - evaluation.Attacks[ENDGAME][TEAM_BLACK] +
			evaluation.PieceSquares[ENDGAME][TEAM_WHITE] - evaluation.PieceSquares[ENDGAME][TEAM_BLACK] +
			evaluation.BlockedPieces[ENDGAME][TEAM_WHITE] - evaluation.BlockedPieces[ENDGAME][TEAM_BLACK] +
			evaluation.DoubledPawns[ENDGAME][TEAM_WHITE] - evaluation.DoubledPawns[ENDGAME][TEAM_BLACK] +
			evaluation.WeakPawns[ENDGAME][TEAM_WHITE] - evaluation.WeakPawns[ENDGAME][TEAM_BLACK] +
			evaluation.PassedPawns[ENDGAME][TEAM_WHITE] - evaluation.PassedPawns[ENDGAME][TEAM_BLACK] +
			evaluation.KingSafety[ENDGAME][TEAM_WHITE] - evaluation.KingSafety[ENDGAME][TEAM_BLACK] +
			evaluation.Tempo[ENDGAME][TEAM_WHITE] - evaluation.Tempo[ENDGAME][TEAM_BLACK],
			scoreLength
		);

		std::string result = "";
		result += "       Term     |     White     |     Black     |     Total     \n";
		result += "                |   MG     EG   |   MG     EG   |   MG     EG   \n";
		result += " ---------------+---------------+---------------+---------------\n";
		result += "       Material | " + FormatScore(evaluation.Material[MIDGAME][TEAM_WHITE], scoreLength)      + " " + FormatScore(evaluation.Material[ENDGAME][TEAM_WHITE], scoreLength)      + " | " + FormatScore(evaluation.Material[MIDGAME][TEAM_BLACK], scoreLength)      + " " + FormatScore(evaluation.Material[ENDGAME][TEAM_BLACK], scoreLength)      + " | " + FormatScore(evaluation.Material[MIDGAME][TEAM_WHITE] - evaluation.Material[MIDGAME][TEAM_BLACK], scoreLength)           + " " + FormatScore(evaluation.Material[MIDGAME][TEAM_WHITE] - evaluation.Material[MIDGAME][TEAM_BLACK], scoreLength)           + '\n';
		result += "        Attacks | " + FormatScore(evaluation.Attacks[MIDGAME][TEAM_WHITE], scoreLength)       + " " + FormatScore(evaluation.Attacks[ENDGAME][TEAM_WHITE], scoreLength)       + " | " + FormatScore(evaluation.Attacks[MIDGAME][TEAM_BLACK], scoreLength)       + " " + FormatScore(evaluation.Attacks[ENDGAME][TEAM_BLACK], scoreLength)       + " | " + FormatScore(evaluation.Attacks[MIDGAME][TEAM_WHITE] - evaluation.Attacks[MIDGAME][TEAM_BLACK], scoreLength)             + " " + FormatScore(evaluation.Attacks[MIDGAME][TEAM_WHITE] - evaluation.Attacks[MIDGAME][TEAM_BLACK], scoreLength)             + '\n';
		result += "  Piece Squares | " + FormatScore(evaluation.PieceSquares[MIDGAME][TEAM_WHITE], scoreLength)  + " " + FormatScore(evaluation.PieceSquares[ENDGAME][TEAM_WHITE], scoreLength)  + " | " + FormatScore(evaluation.PieceSquares[MIDGAME][TEAM_BLACK], scoreLength)  + " " + FormatScore(evaluation.PieceSquares[ENDGAME][TEAM_BLACK], scoreLength)  + " | " + FormatScore(evaluation.PieceSquares[MIDGAME][TEAM_WHITE] - evaluation.PieceSquares[MIDGAME][TEAM_BLACK], scoreLength)   + " " + FormatScore(evaluation.PieceSquares[MIDGAME][TEAM_WHITE] - evaluation.PieceSquares[MIDGAME][TEAM_BLACK], scoreLength)   + '\n';
		result += " Blocked Pieces | " + FormatScore(evaluation.BlockedPieces[MIDGAME][TEAM_WHITE], scoreLength) + " " + FormatScore(evaluation.BlockedPieces[ENDGAME][TEAM_WHITE], scoreLength) + " | " + FormatScore(evaluation.BlockedPieces[MIDGAME][TEAM_BLACK], scoreLength) + " " + FormatScore(evaluation.BlockedPieces[ENDGAME][TEAM_BLACK], scoreLength) + " | " + FormatScore(evaluation.BlockedPieces[MIDGAME][TEAM_WHITE] - evaluation.BlockedPieces[MIDGAME][TEAM_BLACK], scoreLength) + " " + FormatScore(evaluation.BlockedPieces[MIDGAME][TEAM_WHITE] - evaluation.BlockedPieces[MIDGAME][TEAM_BLACK], scoreLength) + '\n';
		result += "  Doubled Pawns | " + FormatScore(evaluation.DoubledPawns[MIDGAME][TEAM_WHITE], scoreLength)  + " " + FormatScore(evaluation.DoubledPawns[ENDGAME][TEAM_WHITE], scoreLength)  + " | " + FormatScore(evaluation.DoubledPawns[MIDGAME][TEAM_BLACK], scoreLength)  + " " + FormatScore(evaluation.DoubledPawns[ENDGAME][TEAM_BLACK], scoreLength)  + " | " + FormatScore(evaluation.DoubledPawns[MIDGAME][TEAM_WHITE] - evaluation.DoubledPawns[MIDGAME][TEAM_BLACK], scoreLength)   + " " + FormatScore(evaluation.DoubledPawns[MIDGAME][TEAM_WHITE] - evaluation.DoubledPawns[MIDGAME][TEAM_BLACK], scoreLength)   + '\n';
		result += "     Weak Pawns | " + FormatScore(evaluation.WeakPawns[MIDGAME][TEAM_WHITE], scoreLength)     + " " + FormatScore(evaluation.WeakPawns[ENDGAME][TEAM_WHITE], scoreLength)     + " | " + FormatScore(evaluation.WeakPawns[MIDGAME][TEAM_BLACK], scoreLength)     + " " + FormatScore(evaluation.WeakPawns[ENDGAME][TEAM_BLACK], scoreLength)     + " | " + FormatScore(evaluation.WeakPawns[MIDGAME][TEAM_WHITE] - evaluation.WeakPawns[MIDGAME][TEAM_BLACK], scoreLength)         + " " + FormatScore(evaluation.WeakPawns[MIDGAME][TEAM_WHITE] - evaluation.WeakPawns[MIDGAME][TEAM_BLACK], scoreLength)         + '\n';
		result += "   Passed Pawns | " + FormatScore(evaluation.PassedPawns[MIDGAME][TEAM_WHITE], scoreLength)   + " " + FormatScore(evaluation.PassedPawns[ENDGAME][TEAM_WHITE], scoreLength)   + " | " + FormatScore(evaluation.PassedPawns[MIDGAME][TEAM_BLACK], scoreLength)   + " " + FormatScore(evaluation.PassedPawns[ENDGAME][TEAM_BLACK], scoreLength)   + " | " + FormatScore(evaluation.PassedPawns[MIDGAME][TEAM_WHITE] - evaluation.PassedPawns[MIDGAME][TEAM_BLACK], scoreLength)     + " " + FormatScore(evaluation.PassedPawns[MIDGAME][TEAM_WHITE] - evaluation.PassedPawns[MIDGAME][TEAM_BLACK], scoreLength)     + '\n';
		result += "    King Safety | " + FormatScore(evaluation.KingSafety[MIDGAME][TEAM_WHITE], scoreLength)    + " " + FormatScore(evaluation.KingSafety[ENDGAME][TEAM_WHITE], scoreLength)    + " | " + FormatScore(evaluation.KingSafety[MIDGAME][TEAM_BLACK], scoreLength)    + " " + FormatScore(evaluation.KingSafety[ENDGAME][TEAM_BLACK], scoreLength)    + " | " + FormatScore(evaluation.KingSafety[MIDGAME][TEAM_WHITE] - evaluation.KingSafety[MIDGAME][TEAM_BLACK], scoreLength)       + " " + FormatScore(evaluation.KingSafety[MIDGAME][TEAM_WHITE] - evaluation.KingSafety[MIDGAME][TEAM_BLACK], scoreLength)       + '\n';
		result += "          Tempo | " + FormatScore(evaluation.Tempo[MIDGAME][TEAM_WHITE], scoreLength)         + " " + FormatScore(evaluation.Tempo[ENDGAME][TEAM_WHITE], scoreLength)         + " | " + FormatScore(evaluation.Tempo[MIDGAME][TEAM_BLACK], scoreLength)         + " " + FormatScore(evaluation.Tempo[ENDGAME][TEAM_BLACK], scoreLength)         + " | " + FormatScore(evaluation.Tempo[MIDGAME][TEAM_WHITE] - evaluation.Tempo[MIDGAME][TEAM_BLACK], scoreLength)                 + " " + FormatScore(evaluation.Tempo[MIDGAME][TEAM_WHITE] - evaluation.Tempo[MIDGAME][TEAM_BLACK], scoreLength)                 + '\n';
		result += " ---------------+---------------+---------------+--------------\n";
		result += "          Total | " + whiteMidgameTotal + " " + whiteEndgameTotal + " | " + blackMidgameTotal + " " + blackEndgameTotal + " | " + midgameTotal + " " + endgameTotal + "\n";
		result += "\n";
		result += "Game stage: " + std::to_string(evaluation.GameStage) + '\n';
		result += "Total evaluation: " + std::to_string(evaluation.GetTotal(TEAM_WHITE));
		if (evaluation.GetTotal(TEAM_WHITE) > 0)
			result += " (white side)";
		else
			result += " (black side)";

		return result;
	}

}
