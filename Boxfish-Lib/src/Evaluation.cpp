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

	static Move s_MoveBuffer[MAX_MOVES];
	static MoveList s_MoveList(s_MoveBuffer);

	static int s_PhaseWeights[PIECE_MAX];
	int s_MaxPhaseValue = 0;

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
		s_PassedPawnRankWeights[MIDGAME][TEAM_BLACK][RANK_2] = 50;
		s_PassedPawnRankWeights[ENDGAME][TEAM_BLACK][RANK_2] = 80;

		s_PassedPawnRankWeights[MIDGAME][TEAM_WHITE][RANK_3] = 10;
		s_PassedPawnRankWeights[ENDGAME][TEAM_WHITE][RANK_3] = 30;
		s_PassedPawnRankWeights[MIDGAME][TEAM_BLACK][RANK_3] = 30;
		s_PassedPawnRankWeights[ENDGAME][TEAM_BLACK][RANK_3] = 40;

		s_PassedPawnRankWeights[MIDGAME][TEAM_WHITE][RANK_4] = 12;
		s_PassedPawnRankWeights[ENDGAME][TEAM_WHITE][RANK_4] = 40;
		s_PassedPawnRankWeights[MIDGAME][TEAM_BLACK][RANK_4] = 15;
		s_PassedPawnRankWeights[ENDGAME][TEAM_BLACK][RANK_4] = 60;

		s_PassedPawnRankWeights[MIDGAME][TEAM_WHITE][RANK_5] = 15;
		s_PassedPawnRankWeights[ENDGAME][TEAM_WHITE][RANK_5] = 60;
		s_PassedPawnRankWeights[MIDGAME][TEAM_BLACK][RANK_5] = 12;
		s_PassedPawnRankWeights[ENDGAME][TEAM_BLACK][RANK_5] = 40;

		s_PassedPawnRankWeights[MIDGAME][TEAM_WHITE][RANK_6] = 30;
		s_PassedPawnRankWeights[ENDGAME][TEAM_WHITE][RANK_6] = 40;
		s_PassedPawnRankWeights[MIDGAME][TEAM_BLACK][RANK_6] = 10;
		s_PassedPawnRankWeights[ENDGAME][TEAM_BLACK][RANK_6] = 30;

		s_PassedPawnRankWeights[MIDGAME][TEAM_WHITE][RANK_7] = 50;
		s_PassedPawnRankWeights[ENDGAME][TEAM_WHITE][RANK_7] = 80;
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
		s_MaterialValues[MIDGAME][PIECE_KNIGHT] = 325;
		s_MaterialValues[MIDGAME][PIECE_BISHOP] = 335;
		s_MaterialValues[MIDGAME][PIECE_ROOK] = 500;
		s_MaterialValues[MIDGAME][PIECE_QUEEN] = 975;
		s_MaterialValues[MIDGAME][PIECE_KING] = 20000;

		s_MaterialValues[ENDGAME][PIECE_PAWN] = 100;
		s_MaterialValues[ENDGAME][PIECE_KNIGHT] = 325;
		s_MaterialValues[ENDGAME][PIECE_BISHOP] = 335;
		s_MaterialValues[ENDGAME][PIECE_ROOK] = 500;
		s_MaterialValues[ENDGAME][PIECE_QUEEN] = 975;
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

	// =================================================================================================================================
	// HELPER FUNCTIONS
	// =================================================================================================================================

	int CalculateGameStage(const Position& position)
	{
		int phase = 0;
		// Pawns and kings aren't included
		for (Piece piece = PIECE_KNIGHT; piece < PIECE_KING; piece++)
		{
			BitBoard pieceBoard = position.GetPieces(piece);
			phase += pieceBoard.GetCount() * s_PhaseWeights[piece];
		}
		phase = std::min(s_MaxPhaseValue, phase);
		return s_MaxPhaseValue - phase;
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

	bool IsPawnSupported(const Position& position, SquareIndex square, Team team)
	{
		const BitBoard& pawns = position.GetTeamPieces(team, PIECE_PAWN);
		return (s_SupportedPawnMasks[team][square] & pawns) != ZERO_BB;
	}

	template<Team team>
	BitBoard CalculatePawnAttacks(const Position& position)
	{
		const BitBoard& pawns = position.GetTeamPieces(team, PIECE_PAWN);
		// Don't & with ~team pieces so that pawns can defend their pieces
		if (team == TEAM_WHITE)
		{
			return (((pawns << 9) & ~FILE_H_MASK) | ((pawns << 7) & ~FILE_A_MASK)) & ~RANK_1_MASK;
		}
		else
		{
			return (((pawns >> 9) & ~FILE_H_MASK) | ((pawns >> 7) & ~FILE_A_MASK)) & ~RANK_8_MASK;
		}
	}

	int CalculateTropism(SquareIndex a, SquareIndex b)
	{
		Square sqA = BitBoard::BitIndexToSquare(a);
		Square sqB = BitBoard::BitIndexToSquare(b);
		return 7 - (std::abs(sqA.Rank - sqB.Rank) + std::abs(sqA.File - sqB.File));
	}

	// =================================================================================================================================
	// EVALUATION FUNCTIONS
	// =================================================================================================================================

	void EvaluateCheckmate(EvaluationResult& result, const Position& position)
	{
		MoveGenerator movegen(position);
		result.Checkmate[TEAM_WHITE] = false;
		result.Checkmate[TEAM_BLACK] = false;
		result.Stalemate = false;
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
	void EvaluateMaterial(EvaluationResult& result, const Position& position)
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

	void EvaluateMaterial(EvaluationResult& result, const Position& position)
	{
		EvaluateMaterial<TEAM_WHITE>(result, position);
		EvaluateMaterial<TEAM_BLACK>(result, position);
	}

	template<Team team>
	void EvaluatePieceSquareTables(EvaluationResult& result, const Position& position)
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

	void EvaluatePieceSquareTables(EvaluationResult& result, const Position& position)
	{
		EvaluatePieceSquareTables<TEAM_WHITE>(result, position);
		EvaluatePieceSquareTables<TEAM_BLACK>(result, position);
	}

	template<Team team>
	void EvaluatePassedPawns(EvaluationResult& result, const Position& position)
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
				Rank rank = BitBoard::RankOfIndex(square);
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

	void EvaluatePassedPawns(EvaluationResult& result, const Position& position)
	{
		EvaluatePassedPawns<TEAM_WHITE>(result, position);
		EvaluatePassedPawns<TEAM_BLACK>(result, position);
	}

	template<Team team>
	void EvaluateDoubledPawns(EvaluationResult& result, const Position& position)
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

	void EvaluateDoubledPawns(EvaluationResult& result, const Position& position)
	{
		EvaluateDoubledPawns<TEAM_WHITE>(result, position);
		EvaluateDoubledPawns<TEAM_BLACK>(result, position);
	}

	template<Team team>
	void EvaluateWeakPawns(EvaluationResult& result, const Position& position)
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

	void EvaluateWeakPawns(EvaluationResult& result, const Position& position)
	{
		EvaluateWeakPawns<TEAM_WHITE>(result, position);
		EvaluateWeakPawns<TEAM_BLACK>(result, position);
	}

	template<Team team>
	void EvaluateBlockedPieces(EvaluationResult& result, const Position& position)
	{
		constexpr Team otherTeam = OtherTeam(team);
		Centipawns mg = 0;
		Centipawns eg = 0;

		constexpr Centipawns TRAPPED_BISHOP_A8[GAME_STAGE_MAX] = { -150, -50 };
		constexpr Centipawns TRAPPED_BISHOP_A7[GAME_STAGE_MAX] = { -100, -50 };
		constexpr Centipawns TRAPPED_BISHOP_A6[GAME_STAGE_MAX] = { -50, -50 };
		constexpr Centipawns TRAPPED_BISHOP_B8[GAME_STAGE_MAX] = { -150, -50 };

		constexpr Centipawns TRAPPED_ROOK[GAME_STAGE_MAX] = { -25, -25 };

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

	void EvaluateBlockedPieces(EvaluationResult& result, const Position& position)
	{
		EvaluateBlockedPieces<TEAM_WHITE>(result, position);
		EvaluateBlockedPieces<TEAM_BLACK>(result, position);
	}

	template<Team team>
	void EvaluateKnights(EvaluationResult& result, const Position& position)
	{
		constexpr Team otherTeam = OtherTeam(team);

		Centipawns mg = 0;
		Centipawns eg = 0;

		BitBoard knights = position.GetTeamPieces(team, PIECE_KNIGHT);
		BitBoard notTeamPieces = ~position.GetTeamPieces(team);
		while (knights)
		{
			SquareIndex square = PopLeastSignificantBit(knights);
			BitBoard attacks = GetNonSlidingAttacks(PIECE_KNIGHT, square, team) & notTeamPieces;

			// Mobility
			int safeSquaresReachable = (attacks & ~result.Data.PawnAttacks[otherTeam]).GetCount();
			mg += 4 * (safeSquaresReachable - 3);
			eg += 4 * (safeSquaresReachable - 3);

			// King safety
			int kingSquaresAttacked = (attacks & result.Data.KingAttackZone[otherTeam]).GetCount();
			if (kingSquaresAttacked > 0)
			{
				result.Data.Attackers[team]++;
				result.Data.AttackUnits[team] += s_AttackUnits[PIECE_KNIGHT] * kingSquaresAttacked;
			}

			int tropism = CalculateTropism(square, position.InfoCache.KingSquare[otherTeam]);
			mg += 3 * tropism;
			eg += 3 * tropism;
		}
		result.Knights[MIDGAME][team] = mg;
		result.Knights[ENDGAME][team] = eg;
	}

	void EvaluateKnights(EvaluationResult& result, const Position& position)
	{
		EvaluateKnights<TEAM_WHITE>(result, position);
		EvaluateKnights<TEAM_BLACK>(result, position);
	}

	template<Team team>
	void EvaluateBishops(EvaluationResult& result, const Position& position)
	{
		constexpr Team otherTeam = OtherTeam(team);

		Centipawns mg = 0;
		Centipawns eg = 0;

		BitBoard bishops = position.GetTeamPieces(team, PIECE_BISHOP);
		BitBoard notTeamPieces = ~position.GetTeamPieces(team);
		BitBoard blockers = position.GetAllPieces();
		while (bishops)
		{
			SquareIndex square = PopLeastSignificantBit(bishops);
			BitBoard attacks = GetSlidingAttacks(PIECE_BISHOP, square, blockers) & notTeamPieces;

			// Mobility
			int safeReachableSquares = (attacks & ~result.Data.PawnAttacks[otherTeam]).GetCount();
			mg += 3 * (safeReachableSquares - 6);
			eg += 3 * (safeReachableSquares - 6);

			// King safety
			int kingSquaresAttacked = (attacks & result.Data.KingAttackZone[otherTeam]).GetCount();
			if (kingSquaresAttacked > 0)
			{
				result.Data.Attackers[team]++;
				result.Data.AttackUnits[team] += s_AttackUnits[PIECE_BISHOP] * kingSquaresAttacked;
			}

			int tropism = CalculateTropism(square, position.InfoCache.KingSquare[otherTeam]);
			mg += 2 * tropism;
			eg += 1 * tropism;
		}
		result.Bishops[MIDGAME][team] = mg;
		result.Bishops[ENDGAME][team] = eg;
	}

	void EvaluateBishops(EvaluationResult& result, const Position& position)
	{
		EvaluateBishops<TEAM_WHITE>(result, position);
		EvaluateBishops<TEAM_BLACK>(result, position);
	}

	template<Team team>
	void EvaluateRooks(EvaluationResult& result, const Position& position)
	{
		constexpr Team otherTeam = OtherTeam(team);

		Centipawns mg = 0;
		Centipawns eg = 0;

		BitBoard rooks = position.GetTeamPieces(team, PIECE_ROOK);
		BitBoard notTeamPieces = ~position.GetTeamPieces(team);
		BitBoard blockers = position.GetAllPieces();
		while (rooks)
		{
			SquareIndex square = PopLeastSignificantBit(rooks);
			BitBoard attacks = GetSlidingAttacks(PIECE_ROOK, square, blockers) & notTeamPieces;

			// Mobility
			int squaresReachable = attacks.GetCount();
			mg += 2 * (squaresReachable - 6);
			eg += 4 * (squaresReachable - 6);

			// King safety
			int kingSquaresAttacked = (attacks & result.Data.KingAttackZone[otherTeam]).GetCount();
			if (kingSquaresAttacked > 0)
			{
				result.Data.Attackers[team]++;
				result.Data.AttackUnits[team] += s_AttackUnits[PIECE_ROOK] * kingSquaresAttacked;
			}

			int tropism = CalculateTropism(square, position.InfoCache.KingSquare[otherTeam]);
			mg += 2 * tropism;
			eg += 1 * tropism;
		}
		result.Rooks[MIDGAME][team] = mg;
		result.Rooks[ENDGAME][team] = eg;
	}

	void EvaluateRooks(EvaluationResult& result, const Position& position)
	{
		EvaluateRooks<TEAM_WHITE>(result, position);
		EvaluateRooks<TEAM_BLACK>(result, position);
	}

	template<Team team>
	void EvaluateQueens(EvaluationResult& result, const Position& position)
	{
		constexpr Team otherTeam = OtherTeam(team);

		Centipawns mg = 0;
		Centipawns eg = 0;

		BitBoard queens = position.GetTeamPieces(team, PIECE_QUEEN);
		BitBoard notTeamPieces = ~position.GetTeamPieces(team);
		BitBoard blockers = position.GetAllPieces();
		while (queens)
		{
			result.Data.HasQueenAttacker[team] = true;
			SquareIndex square = PopLeastSignificantBit(queens);
			BitBoard attacks = GetSlidingAttacks(PIECE_QUEEN, square, blockers) & notTeamPieces;

			// Mobility
			int reachableSquares = attacks.GetCount();
			mg += 1 * (reachableSquares - 13);
			eg += 2 * (reachableSquares - 13);

			// King safety
			int kingSquaresAttacked = (attacks & result.Data.KingAttackZone[otherTeam]).GetCount();
			if (kingSquaresAttacked > 0)
			{
				result.Data.Attackers[team]++;
				result.Data.AttackUnits[team] += s_AttackUnits[PIECE_QUEEN] * kingSquaresAttacked;
			}

			int tropism = CalculateTropism(square, position.InfoCache.KingSquare[otherTeam]);
			mg += 2 * tropism;
			eg += 4 * tropism;
		}
		result.Queens[MIDGAME][team] = mg;
		result.Queens[ENDGAME][team] = eg;
	}

	void EvaluateQueens(EvaluationResult& result, const Position& position)
	{
		EvaluateQueens<TEAM_WHITE>(result, position);
		EvaluateQueens<TEAM_BLACK>(result, position);
	}

	template<Team team>
	void EvaluateKingSafety(EvaluationResult& result, const Position& position)
	{
		constexpr Team otherTeam = OtherTeam(team);
		Square kingSquare = BitBoard::BitIndexToSquare(position.InfoCache.KingSquare[team]);

		Centipawns value = 0;
		if (result.Data.Attackers[otherTeam] > 2 && result.Data.HasQueenAttacker[otherTeam])
		{
			value = -s_KingSafetyTable[std::min(result.Data.AttackUnits[otherTeam], MAX_ATTACK_UNITS - 1)];
		}

		constexpr Centipawns PAWN_SHIELD_1 = 10;
		constexpr Centipawns PAWN_SHIELD_2 = 5;
		const BitBoard& kingZone = result.Data.KingAttackZone[team];
		if (team == TEAM_WHITE && (kingSquare.File > FILE_E || kingSquare.File < FILE_D))
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
		else if (team == TEAM_BLACK && (kingSquare.File > FILE_E || kingSquare.File < FILE_D))
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
		result.KingSafety[ENDGAME][team] = 0;
	}

	void EvaluateKingSafety(EvaluationResult& result, const Position& position)
	{
		EvaluateKingSafety<TEAM_WHITE>(result, position);
		EvaluateKingSafety<TEAM_BLACK>(result, position);
	}

	void EvaluateTempo(EvaluationResult& result, const Position& position)
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

	void ClearResult(EvaluationResult& result)
	{
		memset(&result, 0, sizeof(EvaluationResult));
	}

	EvaluationResult EvaluateDetailed(const Position& position, Team team, Centipawns alpha, Centipawns beta)
	{
		EvaluationResult result;
		ClearResult(result);
		result.GameStage = CalculateGameStage(position);
		EvaluateCheckmate(result, position);
		if (!result.IsCheckmate() && !result.Stalemate)
		{
			EvaluateMaterial(result, position);
			EvaluatePieceSquareTables(result, position);
			
			Centipawns current = result.GetTotal(team);
			constexpr Centipawns delta = 200;
			if (current > alpha - delta && current < beta + delta)
			{
				result.Data.KingAttackZone[TEAM_WHITE] = CalculateKingAttackRegion<TEAM_WHITE>(position);
				result.Data.KingAttackZone[TEAM_BLACK] = CalculateKingAttackRegion<TEAM_BLACK>(position);
				result.Data.PawnAttacks[TEAM_WHITE] = CalculatePawnAttacks<TEAM_WHITE>(position);
				result.Data.PawnAttacks[TEAM_BLACK] = CalculatePawnAttacks<TEAM_BLACK>(position);

				EvaluatePassedPawns(result, position);
				EvaluateDoubledPawns(result, position);
				EvaluateWeakPawns(result, position);
				EvaluateBlockedPieces(result, position);
				EvaluateKnights(result, position);
				EvaluateBishops(result, position);
				EvaluateRooks(result, position);
				EvaluateQueens(result, position);
				EvaluateKingSafety(result, position);
				EvaluateTempo(result, position);
			}
		}
		return result;
	}

	Centipawns Evaluate(const Position& position, Team team, Centipawns alpha, Centipawns beta)
	{
		return EvaluateDetailed(position, team, alpha, beta).GetTotal(team);
	}

	EvaluationResult EvaluateDetailed(const Position& position)
	{
		return EvaluateDetailed(position, TEAM_WHITE, -SCORE_MATE, SCORE_MATE);
	}

	Centipawns Evaluate(const Position& position, Team team)
	{
		return Evaluate(position, team, -SCORE_MATE, SCORE_MATE);
	}

	bool IsEndgame(const Position& position)
	{
		EvaluationResult result;
		EvaluateMaterial(result, position);
		return result.Material[MIDGAME][TEAM_WHITE] + result.Material[MIDGAME][TEAM_BLACK] <= 1300;
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
			evaluation.PieceSquares[MIDGAME][TEAM_WHITE] +
			evaluation.DoubledPawns[MIDGAME][TEAM_WHITE] +
			evaluation.WeakPawns[MIDGAME][TEAM_WHITE] +
			evaluation.PassedPawns[MIDGAME][TEAM_WHITE] +
			evaluation.BlockedPieces[MIDGAME][TEAM_WHITE] +
			evaluation.Knights[MIDGAME][TEAM_WHITE] +
			evaluation.Bishops[MIDGAME][TEAM_WHITE] +
			evaluation.Rooks[MIDGAME][TEAM_WHITE] +
			evaluation.Queens[MIDGAME][TEAM_WHITE] +
			evaluation.KingSafety[MIDGAME][TEAM_WHITE] +
			evaluation.Tempo[MIDGAME][TEAM_WHITE],
			scoreLength
		);

		std::string whiteEndgameTotal = FormatScore(
			evaluation.Material[ENDGAME][TEAM_WHITE] +
			evaluation.PieceSquares[ENDGAME][TEAM_WHITE] +
			evaluation.DoubledPawns[ENDGAME][TEAM_WHITE] +
			evaluation.WeakPawns[ENDGAME][TEAM_WHITE] +
			evaluation.PassedPawns[ENDGAME][TEAM_WHITE] +
			evaluation.BlockedPieces[ENDGAME][TEAM_WHITE] +
			evaluation.Knights[ENDGAME][TEAM_WHITE] +
			evaluation.Bishops[ENDGAME][TEAM_WHITE] +
			evaluation.Rooks[ENDGAME][TEAM_WHITE] +
			evaluation.Queens[ENDGAME][TEAM_WHITE] +
			evaluation.KingSafety[ENDGAME][TEAM_WHITE] +
			evaluation.Tempo[ENDGAME][TEAM_WHITE],
			scoreLength
		);

		std::string blackMidgameTotal = FormatScore(
			evaluation.Material[MIDGAME][TEAM_BLACK] +
			evaluation.PieceSquares[MIDGAME][TEAM_BLACK] +
			evaluation.DoubledPawns[MIDGAME][TEAM_BLACK] +
			evaluation.WeakPawns[MIDGAME][TEAM_BLACK] +
			evaluation.PassedPawns[MIDGAME][TEAM_BLACK] +
			evaluation.BlockedPieces[MIDGAME][TEAM_BLACK] +
			evaluation.Knights[MIDGAME][TEAM_BLACK] +
			evaluation.Bishops[MIDGAME][TEAM_BLACK] +
			evaluation.Rooks[MIDGAME][TEAM_BLACK] +
			evaluation.Queens[MIDGAME][TEAM_BLACK] +
			evaluation.KingSafety[MIDGAME][TEAM_BLACK] +
			evaluation.Tempo[MIDGAME][TEAM_BLACK],
			scoreLength
		);

		std::string blackEndgameTotal = FormatScore(
			evaluation.Material[ENDGAME][TEAM_BLACK] +
			evaluation.PieceSquares[ENDGAME][TEAM_BLACK] +
			evaluation.DoubledPawns[ENDGAME][TEAM_BLACK] +
			evaluation.WeakPawns[ENDGAME][TEAM_BLACK] +
			evaluation.PassedPawns[ENDGAME][TEAM_BLACK] +
			evaluation.BlockedPieces[ENDGAME][TEAM_BLACK] +
			evaluation.Knights[ENDGAME][TEAM_BLACK] +
			evaluation.Bishops[ENDGAME][TEAM_BLACK] +
			evaluation.Rooks[ENDGAME][TEAM_BLACK] +
			evaluation.Queens[ENDGAME][TEAM_BLACK] +
			evaluation.KingSafety[ENDGAME][TEAM_BLACK] +
			evaluation.Tempo[ENDGAME][TEAM_BLACK],
			scoreLength
		);

		std::string midgameTotal = FormatScore(
			evaluation.Material[MIDGAME][TEAM_WHITE] - evaluation.Material[MIDGAME][TEAM_BLACK] +
			evaluation.PieceSquares[MIDGAME][TEAM_WHITE] - evaluation.PieceSquares[MIDGAME][TEAM_BLACK] +
			evaluation.DoubledPawns[MIDGAME][TEAM_WHITE] - evaluation.DoubledPawns[MIDGAME][TEAM_BLACK] +
			evaluation.WeakPawns[MIDGAME][TEAM_WHITE] - evaluation.WeakPawns[MIDGAME][TEAM_BLACK] +
			evaluation.PassedPawns[MIDGAME][TEAM_WHITE] - evaluation.PassedPawns[MIDGAME][TEAM_BLACK] +
			evaluation.BlockedPieces[MIDGAME][TEAM_WHITE] - evaluation.BlockedPieces[MIDGAME][TEAM_BLACK] +
			evaluation.Knights[MIDGAME][TEAM_WHITE] - evaluation.Knights[MIDGAME][TEAM_BLACK] +
			evaluation.Bishops[MIDGAME][TEAM_WHITE] - evaluation.Bishops[MIDGAME][TEAM_BLACK] +
			evaluation.Rooks[MIDGAME][TEAM_WHITE] - evaluation.Rooks[MIDGAME][TEAM_BLACK] +
			evaluation.Queens[MIDGAME][TEAM_WHITE] - evaluation.Queens[MIDGAME][TEAM_BLACK] +
			evaluation.KingSafety[MIDGAME][TEAM_WHITE] - evaluation.KingSafety[MIDGAME][TEAM_BLACK] +
			evaluation.Tempo[MIDGAME][TEAM_WHITE] - evaluation.Tempo[MIDGAME][TEAM_BLACK],
			scoreLength
		);

		std::string endgameTotal = FormatScore(
			evaluation.Material[ENDGAME][TEAM_WHITE] - evaluation.Material[ENDGAME][TEAM_BLACK] +
			evaluation.PieceSquares[ENDGAME][TEAM_WHITE] - evaluation.PieceSquares[ENDGAME][TEAM_BLACK] +
			evaluation.DoubledPawns[ENDGAME][TEAM_WHITE] - evaluation.DoubledPawns[ENDGAME][TEAM_BLACK] +
			evaluation.WeakPawns[ENDGAME][TEAM_WHITE] - evaluation.WeakPawns[ENDGAME][TEAM_BLACK] +
			evaluation.PassedPawns[ENDGAME][TEAM_WHITE] - evaluation.PassedPawns[ENDGAME][TEAM_BLACK] +
			evaluation.BlockedPieces[ENDGAME][TEAM_WHITE] - evaluation.BlockedPieces[ENDGAME][TEAM_BLACK] +
			evaluation.Knights[ENDGAME][TEAM_WHITE] - evaluation.Knights[ENDGAME][TEAM_BLACK] +
			evaluation.Bishops[ENDGAME][TEAM_WHITE] - evaluation.Bishops[ENDGAME][TEAM_BLACK] +
			evaluation.Rooks[ENDGAME][TEAM_WHITE] - evaluation.Rooks[ENDGAME][TEAM_BLACK] +
			evaluation.Queens[ENDGAME][TEAM_WHITE] - evaluation.Queens[ENDGAME][TEAM_BLACK] +
			evaluation.KingSafety[ENDGAME][TEAM_WHITE] - evaluation.KingSafety[ENDGAME][TEAM_BLACK] +
			evaluation.Tempo[ENDGAME][TEAM_WHITE] - evaluation.Tempo[ENDGAME][TEAM_BLACK],
			scoreLength
		);

		std::string result = "";
		result += "       Term     |     White     |     Black     |     Total     \n";
		result += "                |   MG     EG   |   MG     EG   |   MG     EG   \n";
		result += " ---------------+---------------+---------------+---------------\n";
		result += "       Material | " + FormatScore(evaluation.Material[MIDGAME][TEAM_WHITE], scoreLength)      + " " + FormatScore(evaluation.Material[ENDGAME][TEAM_WHITE], scoreLength)      + " | " + FormatScore(evaluation.Material[MIDGAME][TEAM_BLACK], scoreLength)      + " " + FormatScore(evaluation.Material[ENDGAME][TEAM_BLACK], scoreLength)      + " | " + FormatScore(evaluation.Material[MIDGAME][TEAM_WHITE] - evaluation.Material[MIDGAME][TEAM_BLACK], scoreLength)           + " " + FormatScore(evaluation.Material[ENDGAME][TEAM_WHITE] - evaluation.Material[ENDGAME][TEAM_BLACK], scoreLength)           + '\n';
		result += "  Piece Squares | " + FormatScore(evaluation.PieceSquares[MIDGAME][TEAM_WHITE], scoreLength)  + " " + FormatScore(evaluation.PieceSquares[ENDGAME][TEAM_WHITE], scoreLength)  + " | " + FormatScore(evaluation.PieceSquares[MIDGAME][TEAM_BLACK], scoreLength)  + " " + FormatScore(evaluation.PieceSquares[ENDGAME][TEAM_BLACK], scoreLength)  + " | " + FormatScore(evaluation.PieceSquares[MIDGAME][TEAM_WHITE] - evaluation.PieceSquares[MIDGAME][TEAM_BLACK], scoreLength)   + " " + FormatScore(evaluation.PieceSquares[ENDGAME][TEAM_WHITE] - evaluation.PieceSquares[ENDGAME][TEAM_BLACK], scoreLength)   + '\n';
		result += " Blocked Pieces | " + FormatScore(evaluation.BlockedPieces[MIDGAME][TEAM_WHITE], scoreLength) + " " + FormatScore(evaluation.BlockedPieces[ENDGAME][TEAM_WHITE], scoreLength) + " | " + FormatScore(evaluation.BlockedPieces[MIDGAME][TEAM_BLACK], scoreLength) + " " + FormatScore(evaluation.BlockedPieces[ENDGAME][TEAM_BLACK], scoreLength) + " | " + FormatScore(evaluation.BlockedPieces[MIDGAME][TEAM_WHITE] - evaluation.BlockedPieces[MIDGAME][TEAM_BLACK], scoreLength) + " " + FormatScore(evaluation.BlockedPieces[ENDGAME][TEAM_WHITE] - evaluation.BlockedPieces[ENDGAME][TEAM_BLACK], scoreLength) + '\n';
		result += "  Doubled Pawns | " + FormatScore(evaluation.DoubledPawns[MIDGAME][TEAM_WHITE], scoreLength)  + " " + FormatScore(evaluation.DoubledPawns[ENDGAME][TEAM_WHITE], scoreLength)  + " | " + FormatScore(evaluation.DoubledPawns[MIDGAME][TEAM_BLACK], scoreLength)  + " " + FormatScore(evaluation.DoubledPawns[ENDGAME][TEAM_BLACK], scoreLength)  + " | " + FormatScore(evaluation.DoubledPawns[MIDGAME][TEAM_WHITE] - evaluation.DoubledPawns[MIDGAME][TEAM_BLACK], scoreLength)   + " " + FormatScore(evaluation.DoubledPawns[ENDGAME][TEAM_WHITE] - evaluation.DoubledPawns[ENDGAME][TEAM_BLACK], scoreLength)   + '\n';
		result += "     Weak Pawns | " + FormatScore(evaluation.WeakPawns[MIDGAME][TEAM_WHITE], scoreLength)     + " " + FormatScore(evaluation.WeakPawns[ENDGAME][TEAM_WHITE], scoreLength)     + " | " + FormatScore(evaluation.WeakPawns[MIDGAME][TEAM_BLACK], scoreLength)     + " " + FormatScore(evaluation.WeakPawns[ENDGAME][TEAM_BLACK], scoreLength)     + " | " + FormatScore(evaluation.WeakPawns[MIDGAME][TEAM_WHITE] - evaluation.WeakPawns[MIDGAME][TEAM_BLACK], scoreLength)         + " " + FormatScore(evaluation.WeakPawns[ENDGAME][TEAM_WHITE] - evaluation.WeakPawns[ENDGAME][TEAM_BLACK], scoreLength)         + '\n';
		result += "   Passed Pawns | " + FormatScore(evaluation.PassedPawns[MIDGAME][TEAM_WHITE], scoreLength)   + " " + FormatScore(evaluation.PassedPawns[ENDGAME][TEAM_WHITE], scoreLength)   + " | " + FormatScore(evaluation.PassedPawns[MIDGAME][TEAM_BLACK], scoreLength)   + " " + FormatScore(evaluation.PassedPawns[ENDGAME][TEAM_BLACK], scoreLength)   + " | " + FormatScore(evaluation.PassedPawns[MIDGAME][TEAM_WHITE] - evaluation.PassedPawns[MIDGAME][TEAM_BLACK], scoreLength)     + " " + FormatScore(evaluation.PassedPawns[ENDGAME][TEAM_WHITE] - evaluation.PassedPawns[ENDGAME][TEAM_BLACK], scoreLength)     + '\n';
		result += "        Knights | " + FormatScore(evaluation.Knights[MIDGAME][TEAM_WHITE], scoreLength)       + " " + FormatScore(evaluation.Knights[ENDGAME][TEAM_WHITE], scoreLength)       + " | " + FormatScore(evaluation.Knights[MIDGAME][TEAM_BLACK], scoreLength)       + " " + FormatScore(evaluation.Knights[ENDGAME][TEAM_BLACK], scoreLength)       + " | " + FormatScore(evaluation.Knights[MIDGAME][TEAM_WHITE] - evaluation.Knights[MIDGAME][TEAM_BLACK], scoreLength)             + " " + FormatScore(evaluation.Knights[ENDGAME][TEAM_WHITE] - evaluation.Knights[ENDGAME][TEAM_BLACK], scoreLength)             + '\n';
		result += "        Bishops | " + FormatScore(evaluation.Bishops[MIDGAME][TEAM_WHITE], scoreLength)       + " " + FormatScore(evaluation.Bishops[ENDGAME][TEAM_WHITE], scoreLength)       + " | " + FormatScore(evaluation.Bishops[MIDGAME][TEAM_BLACK], scoreLength)       + " " + FormatScore(evaluation.Bishops[ENDGAME][TEAM_BLACK], scoreLength)       + " | " + FormatScore(evaluation.Bishops[MIDGAME][TEAM_WHITE] - evaluation.Bishops[MIDGAME][TEAM_BLACK], scoreLength)             + " " + FormatScore(evaluation.Bishops[ENDGAME][TEAM_WHITE] - evaluation.Bishops[ENDGAME][TEAM_BLACK], scoreLength)             + '\n';
		result += "          Rooks | " + FormatScore(evaluation.Rooks[MIDGAME][TEAM_WHITE], scoreLength)         + " " + FormatScore(evaluation.Rooks[ENDGAME][TEAM_WHITE], scoreLength)         + " | " + FormatScore(evaluation.Rooks[MIDGAME][TEAM_BLACK], scoreLength)         + " " + FormatScore(evaluation.Rooks[ENDGAME][TEAM_BLACK], scoreLength)         + " | " + FormatScore(evaluation.Rooks[MIDGAME][TEAM_WHITE] - evaluation.Rooks[MIDGAME][TEAM_BLACK], scoreLength)                 + " " + FormatScore(evaluation.Rooks[ENDGAME][TEAM_WHITE] - evaluation.Rooks[ENDGAME][TEAM_BLACK], scoreLength)                 + '\n';
		result += "         Queens | " + FormatScore(evaluation.Queens[MIDGAME][TEAM_WHITE], scoreLength)        + " " + FormatScore(evaluation.Queens[ENDGAME][TEAM_WHITE], scoreLength)        + " | " + FormatScore(evaluation.Queens[MIDGAME][TEAM_BLACK], scoreLength)        + " " + FormatScore(evaluation.Queens[ENDGAME][TEAM_BLACK], scoreLength)        + " | " + FormatScore(evaluation.Queens[MIDGAME][TEAM_WHITE] - evaluation.Queens[MIDGAME][TEAM_BLACK], scoreLength)               + " " + FormatScore(evaluation.Queens[ENDGAME][TEAM_WHITE] - evaluation.Queens[ENDGAME][TEAM_BLACK], scoreLength)               + '\n';
		result += "    King Safety | " + FormatScore(evaluation.KingSafety[MIDGAME][TEAM_WHITE], scoreLength)    + " " + FormatScore(evaluation.KingSafety[ENDGAME][TEAM_WHITE], scoreLength)    + " | " + FormatScore(evaluation.KingSafety[MIDGAME][TEAM_BLACK], scoreLength)    + " " + FormatScore(evaluation.KingSafety[ENDGAME][TEAM_BLACK], scoreLength)    + " | " + FormatScore(evaluation.KingSafety[MIDGAME][TEAM_WHITE] - evaluation.KingSafety[MIDGAME][TEAM_BLACK], scoreLength)       + " " + FormatScore(evaluation.KingSafety[ENDGAME][TEAM_WHITE] - evaluation.KingSafety[ENDGAME][TEAM_BLACK], scoreLength)       + '\n';
		result += "          Tempo | " + FormatScore(evaluation.Tempo[MIDGAME][TEAM_WHITE], scoreLength)         + " " + FormatScore(evaluation.Tempo[ENDGAME][TEAM_WHITE], scoreLength)         + " | " + FormatScore(evaluation.Tempo[MIDGAME][TEAM_BLACK], scoreLength)         + " " + FormatScore(evaluation.Tempo[ENDGAME][TEAM_BLACK], scoreLength)         + " | " + FormatScore(evaluation.Tempo[MIDGAME][TEAM_WHITE] - evaluation.Tempo[MIDGAME][TEAM_BLACK], scoreLength)                 + " " + FormatScore(evaluation.Tempo[ENDGAME][TEAM_WHITE] - evaluation.Tempo[ENDGAME][TEAM_BLACK], scoreLength)                 + '\n';
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
