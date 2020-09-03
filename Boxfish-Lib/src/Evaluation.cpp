#include "Evaluation.h"
#include "MoveGenerator.h"
#include "Attacks.h"

namespace Boxfish
{

	// =================================================================================================================================
	// INITIALIZATION
	// =================================================================================================================================

	static bool s_Initialized = false;

	constexpr BitBoard QueenSide = FILE_A_MASK | FILE_B_MASK | FILE_C_MASK | FILE_D_MASK;
	constexpr BitBoard CenterFiles = FILE_C_MASK | FILE_D_MASK | FILE_E_MASK | FILE_F_MASK;
	constexpr BitBoard KingSide = FILE_E_MASK | FILE_F_MASK | FILE_G_MASK | FILE_H_MASK;
	constexpr BitBoard Center = (FILE_D_MASK | FILE_E_MASK) & (RANK_4_MASK | RANK_5_MASK);

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

	static constexpr Centipawns s_KnightAdjust[9] = { -20, -16, -12, -8, -4, 0, 4, 8, 12 };
	static constexpr Centipawns s_RookAdjust[9] = { 15, 12, 9, 6, 3, 0, -3, -6, -9 };
 
	static BitBoard s_PawnShieldMasks[TEAM_MAX][FILE_MAX * RANK_MAX];
	static BitBoard s_PassedPawnMasks[TEAM_MAX][FILE_MAX * RANK_MAX];
	static Centipawns s_PassedPawnRankWeights[GAME_STAGE_MAX][TEAM_MAX][RANK_MAX];
	static BitBoard s_SupportedPawnMasks[TEAM_MAX][FILE_MAX * RANK_MAX];

	static BitBoard s_KingRings[TEAM_MAX][FILE_MAX * RANK_MAX];

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

	// Pawns can never be on the 8th rank (1st rank used as a sentinel for when there is no pawn)
	static constexpr Centipawns s_KingShieldStength[FILE_MAX / 2][RANK_MAX] = {
		{ -3, 40, 45, 26, 20, 9, 13 },
		{ -21, 29, 17, -25, -14, -5, -32 },
		{ -5, 36, 12, -1, 16, 1, -22 },
		{ -20, -7, -14, -15, -24, -33, -88 },
	};

	// Pawns can never be on the 8th rank (1st rank used as a sentinel for when there is no pawn)
	static constexpr Centipawns s_PawnStormStrength[FILE_MAX / 2][RANK_MAX] = {
		{ 45, 52, 61, 45, 27, 23, 25 },
		{ 22, -9, 62, 23, 18, -3, 11 },
		{ 2, 25, 81, 17, 3, -6, -1 },
		{ -5, -7, 43, 7, 1, -3, -7 },
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
		for (SquareIndex square = a1; square < FILE_MAX * RANK_MAX; square++)
		{
			s_PawnShieldMasks[TEAM_WHITE][square] = ((square << 8) | ((square << 7) & ~FILE_H_MASK) | ((square << 9) & ~FILE_A_MASK)) & (RANK_2_MASK | RANK_3_MASK);
			s_PawnShieldMasks[TEAM_BLACK][square] = ((square >> 8) | ((square >> 7) & ~FILE_H_MASK) | ((square >> 9) & ~FILE_A_MASK)) & (RANK_7_MASK | RANK_6_MASK);
		}
	}

	void InitPassedPawnMasks()
	{
		for (SquareIndex square = a1; square < FILE_MAX * RANK_MAX; square++)
		{
			BitBoard north = GetRay(RAY_NORTH, square);
			BitBoard south = GetRay(RAY_SOUTH, square);
			
			s_PassedPawnMasks[TEAM_WHITE][square] = north | ShiftEast(north, 1) | ShiftWest(north, 1);
			s_PassedPawnMasks[TEAM_BLACK][square] = south | ShiftWest(south, 1) | ShiftEast(south, 1);

			s_SupportedPawnMasks[TEAM_WHITE][square] = (((square << 1) & ~FILE_A_MASK) | ((square >> 1) & ~FILE_H_MASK) | ((square >> 9) & ~RANK_8_MASK & ~FILE_H_MASK) | ((square >> 7) & ~RANK_8_MASK & ~FILE_A_MASK));
			s_SupportedPawnMasks[TEAM_BLACK][square] = (((square << 1) & ~FILE_A_MASK) | ((square >> 1) & ~FILE_H_MASK) | ((square << 9) & ~RANK_1_MASK & ~FILE_A_MASK) | ((square << 7) & ~RANK_1_MASK & ~FILE_H_MASK));
		}

		s_PassedPawnRankWeights[MIDGAME][TEAM_WHITE][RANK_1] = 0;
		s_PassedPawnRankWeights[ENDGAME][TEAM_WHITE][RANK_1] = 0;
		s_PassedPawnRankWeights[MIDGAME][TEAM_WHITE][RANK_2] = 2;
		s_PassedPawnRankWeights[ENDGAME][TEAM_WHITE][RANK_2] = 9;
		s_PassedPawnRankWeights[MIDGAME][TEAM_WHITE][RANK_3] = 6;
		s_PassedPawnRankWeights[ENDGAME][TEAM_WHITE][RANK_3] = 12;
		s_PassedPawnRankWeights[MIDGAME][TEAM_WHITE][RANK_4] = 5;
		s_PassedPawnRankWeights[ENDGAME][TEAM_WHITE][RANK_4] = 15;
		s_PassedPawnRankWeights[MIDGAME][TEAM_WHITE][RANK_5] = 28;
		s_PassedPawnRankWeights[ENDGAME][TEAM_WHITE][RANK_5] = 31;
		s_PassedPawnRankWeights[MIDGAME][TEAM_WHITE][RANK_6] = 82;
		s_PassedPawnRankWeights[ENDGAME][TEAM_WHITE][RANK_6] = 84;
		s_PassedPawnRankWeights[MIDGAME][TEAM_WHITE][RANK_7] = 135;
		s_PassedPawnRankWeights[ENDGAME][TEAM_WHITE][RANK_7] = 125;
		s_PassedPawnRankWeights[MIDGAME][TEAM_WHITE][RANK_8] = 0;
		s_PassedPawnRankWeights[ENDGAME][TEAM_WHITE][RANK_8] = 0;
		
		s_PassedPawnRankWeights[MIDGAME][TEAM_BLACK][RANK_8] = 0;
		s_PassedPawnRankWeights[ENDGAME][TEAM_BLACK][RANK_8] = 0;
		s_PassedPawnRankWeights[MIDGAME][TEAM_BLACK][RANK_7] = 2;
		s_PassedPawnRankWeights[ENDGAME][TEAM_BLACK][RANK_7] = 9;
		s_PassedPawnRankWeights[MIDGAME][TEAM_BLACK][RANK_6] = 6;
		s_PassedPawnRankWeights[ENDGAME][TEAM_BLACK][RANK_6] = 12;
		s_PassedPawnRankWeights[MIDGAME][TEAM_BLACK][RANK_5] = 5;
		s_PassedPawnRankWeights[ENDGAME][TEAM_BLACK][RANK_5] = 15;
		s_PassedPawnRankWeights[MIDGAME][TEAM_BLACK][RANK_4] = 28;
		s_PassedPawnRankWeights[ENDGAME][TEAM_BLACK][RANK_4] = 31;
		s_PassedPawnRankWeights[MIDGAME][TEAM_BLACK][RANK_3] = 82;
		s_PassedPawnRankWeights[ENDGAME][TEAM_BLACK][RANK_3] = 84;
		s_PassedPawnRankWeights[MIDGAME][TEAM_BLACK][RANK_2] = 135;
		s_PassedPawnRankWeights[ENDGAME][TEAM_BLACK][RANK_2] = 125;
		s_PassedPawnRankWeights[MIDGAME][TEAM_BLACK][RANK_1] = 0;
		s_PassedPawnRankWeights[ENDGAME][TEAM_BLACK][RANK_1] = 0;
	}

	void InitMaterialTable()
	{
		s_MaterialValues[MIDGAME][PIECE_PAWN] = 100;
		s_MaterialValues[MIDGAME][PIECE_KNIGHT] = 325;
		s_MaterialValues[MIDGAME][PIECE_BISHOP] = 335;
		s_MaterialValues[MIDGAME][PIECE_ROOK] = 500;
		s_MaterialValues[MIDGAME][PIECE_QUEEN] = 925;
		s_MaterialValues[MIDGAME][PIECE_KING] = 20000;

		s_MaterialValues[ENDGAME][PIECE_PAWN] = 140;
		s_MaterialValues[ENDGAME][PIECE_KNIGHT] = 325;
		s_MaterialValues[ENDGAME][PIECE_BISHOP] = 340;
		s_MaterialValues[ENDGAME][PIECE_ROOK] = 500;
		s_MaterialValues[ENDGAME][PIECE_QUEEN] = 925;
		s_MaterialValues[ENDGAME][PIECE_KING] = 20000;
	}

	void InitPieceSquareTables()
	{
		Centipawns whitePawnsTable[FILE_MAX * RANK_MAX] = {
			 0,  0,  0,  0,  0,  0,  0,  0,
			50, 50, 50, 50, 50, 50, 50, 50,
			10, 10, 20, 30, 30, 20, 10, 10,
			 5,  5, 10, 25, 25, 10,  5,  5,
			 0,  0,  0, 22, 22,  0,  0,  0,
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

	void InitKingRings()
	{
		for (SquareIndex square = a1; square < FILE_MAX * RANK_MAX; square++)
		{
			File file = BitBoard::FileOfIndex(square);
			Rank rank = BitBoard::RankOfIndex(square);

			BitBoard files = FILE_MASKS[file] | s_NeighbourFiles[file];

			BitBoard ring = files & RANK_MASKS[rank];
			ring |= Shift<SOUTH>(ring) | Shift<NORTH>(ring);

			BitBoard whiteRing = ring;
			if (rank == RANK_1)
			{
				whiteRing |= Shift<NORTH>(whiteRing);
			}
			BitBoard blackRing = ring;
			if (rank == RANK_8)
			{
				blackRing |= Shift<SOUTH>(blackRing);
			}

			s_KingRings[TEAM_WHITE][square] = whiteRing;
			s_KingRings[TEAM_BLACK][square] = blackRing;
		}
	}

	void InitDistanceTable()
	{
		for (SquareIndex i = a1; i < FILE_MAX * RANK_MAX; i++)
		{
			for (SquareIndex j = a1; j < FILE_MAX * RANK_MAX; j++)
			{
				Square sqi = BitBoard::BitIndexToSquare(i);
				Square sqj = BitBoard::BitIndexToSquare(j);
				s_DistanceTable[i][j] = 14 - (std::abs(sqi.File - sqj.File) + std::abs(sqi.Rank - sqi.Rank));
			}
		}
	}

	void InitSafetyTables()
	{
		s_AttackUnits[PIECE_PAWN] = 0;
		s_AttackUnits[PIECE_KNIGHT] = 7;
		s_AttackUnits[PIECE_BISHOP] = 3;
		s_AttackUnits[PIECE_ROOK] = 5;
		s_AttackUnits[PIECE_QUEEN] = 6;
		s_AttackUnits[PIECE_KING] = 0;
	}

	void InitEvaluation()
	{
		if (!s_Initialized)
		{
			InitPhase();
			InitDistanceTable();
			InitSafetyTables();
			InitKingRings();
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
		SquareIndex kingSquare = position.InfoCache.KingSquare[team];
		const BitBoard& kingRing = s_KingRings[team][kingSquare];
		if (BitBoard::FileOfIndex(kingSquare) == FILE_H)
			return kingRing | Shift<WEST>(kingRing);
		if (BitBoard::FileOfIndex(kingSquare) == FILE_A)
			return kingRing | Shift<EAST>(kingRing);
		return kingRing;
	}

	bool IsPawnSupported(const Position& position, SquareIndex square, Team team)
	{
		return (s_SupportedPawnMasks[team][square] & position.GetTeamPieces(team, PIECE_PAWN)) != ZERO_BB;
	}

	template<Team team>
	BitBoard CalculatePawnAttacks(const Position& position)
	{
		const BitBoard& pawns = position.GetTeamPieces(team, PIECE_PAWN);
		// Don't & with ~team pieces so that pawns can defend their pieces
		if (team == TEAM_WHITE)
		{
			return (((pawns << 9) & ~FILE_A_MASK) | ((pawns << 7) & ~FILE_H_MASK)) & ~RANK_1_MASK;
		}
		else
		{
			return (((pawns >> 9) & ~FILE_H_MASK) | ((pawns >> 7) & ~FILE_A_MASK)) & ~RANK_8_MASK;
		}
	}

	int CalculateTropism(SquareIndex a, SquareIndex b)
	{
		/*Square sqA = BitBoard::BitIndexToSquare(a);
		Square sqB = BitBoard::BitIndexToSquare(b);
		return 7 - (std::abs(sqA.Rank - sqB.Rank) + std::abs(sqA.File - sqB.File));*/
		return s_DistanceTable[a][b];
	}

	// =================================================================================================================================
	// EVALUATION FUNCTIONS
	// =================================================================================================================================

	template<Team team>
	void EvaluateMaterial(EvaluationResult& result, const Position& position)
	{
		Centipawns mg = 0;
		Centipawns eg = 0;
		// Don't evaluate material of king pieces
		for (Piece piece = PIECE_PAWN; piece < PIECE_KING; piece++)
		{
			int count = position.GetTeamPieces(team, piece).GetCount();
			mg += s_MaterialValues[MIDGAME][piece] * count;
			eg += s_MaterialValues[ENDGAME][piece] * count;
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

		constexpr Centipawns supportedBonus = 20;

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
		Centipawns eg = -20 * count;
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
		result.WeakPawns[MIDGAME][team] = count * -5;
		result.WeakPawns[ENDGAME][team] = count * -15;
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

		result.Data.AttackedBy[team][PIECE_KNIGHT] = ZERO_BB;

		Centipawns mg = 0;
		Centipawns eg = 0;

		BitBoard knights = position.GetTeamPieces(team, PIECE_KNIGHT);
		BitBoard notTeamPieces = ~position.GetTeamPieces(team);

		int pawnCount = position.GetTeamPieces(team, PIECE_PAWN).GetCount();
		mg += s_KnightAdjust[pawnCount];
		eg += s_KnightAdjust[pawnCount];

		while (knights)
		{
			SquareIndex square = PopLeastSignificantBit(knights);
			BitBoard attacks = GetNonSlidingAttacks(PIECE_KNIGHT, square, team) & notTeamPieces;
			result.Data.AttackedBy[team][PIECE_KNIGHT] |= attacks;
			result.Data.AttackedByTwice[team] |= result.Data.AttackedBy[team][PIECE_ALL] & attacks;
			result.Data.AttackedBy[team][PIECE_ALL] |= attacks;

			// Mobility
			int safeSquaresReachable = (attacks & ~result.Data.AttackedBy[otherTeam][PIECE_PAWN]).GetCount();
			mg += 4 * (safeSquaresReachable - 4);
			eg += 4 * (safeSquaresReachable - 4);

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

		result.Data.AttackedBy[team][PIECE_BISHOP] = ZERO_BB;

		BitBoard bishops = position.GetTeamPieces(team, PIECE_BISHOP);
		BitBoard notTeamPieces = ~position.GetTeamPieces(team);
		BitBoard blockers = position.GetAllPieces();
		while (bishops)
		{
			SquareIndex square = PopLeastSignificantBit(bishops);
			BitBoard attacks = GetSlidingAttacks(PIECE_BISHOP, square, blockers) & notTeamPieces;
			result.Data.AttackedBy[team][PIECE_BISHOP] |= attacks;
			result.Data.AttackedByTwice[team] |= result.Data.AttackedBy[team][PIECE_ALL] & attacks;
			result.Data.AttackedBy[team][PIECE_ALL] |= attacks;

			if (MoreThanOne(GetSlidingAttacks(PIECE_BISHOP, square, position.GetPieces(PIECE_PAWN)) & Center))
				mg += 20;

			// Mobility
			int safeReachableSquares = (attacks & ~result.Data.AttackedBy[otherTeam][PIECE_PAWN]).GetCount();
			mg += 3 * (safeReachableSquares - 5);
			eg += 3 * (safeReachableSquares - 5);

			// King safety
			int kingSquaresAttacked = (attacks & result.Data.KingAttackZone[otherTeam]).GetCount();
			if (kingSquaresAttacked > 0)
			{
				result.Data.Attackers[team]++;
				result.Data.AttackUnits[team] += s_AttackUnits[PIECE_BISHOP] * kingSquaresAttacked;
			}

			int tropism = CalculateTropism(square, position.GetKingSquare(otherTeam));
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

		result.Data.AttackedBy[team][PIECE_ROOK] = ZERO_BB;

		BitBoard rooks = position.GetTeamPieces(team, PIECE_ROOK);
		BitBoard notTeamPieces = ~position.GetTeamPieces(team);
		BitBoard blockers = position.GetAllPieces();

		int pawnCount = position.GetTeamPieces(team, PIECE_PAWN).GetCount();
		mg += s_RookAdjust[pawnCount];
		eg += s_RookAdjust[pawnCount];

		BitBoard pawns = position.GetPieces(PIECE_PAWN);
			
		while (rooks)
		{
			SquareIndex square = PopLeastSignificantBit(rooks);
			BitBoard attacks = GetSlidingAttacks(PIECE_ROOK, square, blockers) & notTeamPieces;
			result.Data.AttackedBy[team][PIECE_ROOK] |= attacks;
			result.Data.AttackedByTwice[team] |= result.Data.AttackedBy[team][PIECE_ALL] & attacks;
			result.Data.AttackedBy[team][PIECE_ALL] |= attacks;

			int pawnCountOnFile = (FILE_MASKS[BitBoard::FileOfIndex(square)] & pawns).GetCount();
			if (pawnCountOnFile < 2)
			{
				mg += 20 * (2 - pawnCountOnFile);
				eg += 15 * (2 - pawnCountOnFile);
			}

			// Mobility
			int squaresReachable = (attacks & ~result.Data.AttackedBy[otherTeam][PIECE_PAWN]).GetCount();
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

		result.Data.AttackedBy[team][PIECE_QUEEN] = ZERO_BB;

		Centipawns mg = 0;
		Centipawns eg = 0;

		BitBoard queens = position.GetTeamPieces(team, PIECE_QUEEN);
		BitBoard notTeamPieces = ~position.GetTeamPieces(team);
		BitBoard blockers = position.GetAllPieces();
		while (queens)
		{
			SquareIndex square = PopLeastSignificantBit(queens);
			BitBoard attacks = GetSlidingAttacks(PIECE_QUEEN, square, blockers) & notTeamPieces;
			result.Data.AttackedBy[team][PIECE_QUEEN] |= attacks;
			result.Data.AttackedByTwice[team] |= result.Data.AttackedBy[team][PIECE_ALL] & attacks;
			result.Data.AttackedBy[team][PIECE_ALL] |= attacks;

			if (GetSliderBlockers(position, position.GetTeamPieces(otherTeam, PIECE_ROOK, PIECE_BISHOP), square, nullptr))
			{
				mg -= 25;
				eg -= 5;
			}

			// Mobility
			int reachableSquares = (attacks & ~result.Data.AttackedBy[otherTeam][PIECE_PAWN]).GetCount();
			mg += 1 * (reachableSquares - 12);
			eg += 2 * (reachableSquares - 12);

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

		SquareIndex kngSq = position.GetKingSquare(team);
		Square kingSquare = BitBoard::BitIndexToSquare(kngSq);

		result.Data.Attackers[otherTeam] += (result.Data.KingAttackZone[team] & result.Data.AttackedBy[otherTeam][PIECE_PAWN]).GetCount();

		Centipawns mg = 0;

		if (result.Data.Attackers[otherTeam] > 2 && position.GetTeamPieces(otherTeam, PIECE_QUEEN).GetCount() > 0)
		{
			mg -= s_KingSafetyTable[std::min(result.Data.AttackUnits[otherTeam], MAX_ATTACK_UNITS - 1)];
		}

		// King Shield
		File kingFileCenter = std::min(std::max(kingSquare.File, FILE_B), FILE_G);
		for (File file = (File)(kingFileCenter - 1); file <= kingFileCenter + 1; file++)
		{
			BitBoard ourPawns = FILE_MASKS[file] & position.GetTeamPieces(team, PIECE_PAWN);
			BitBoard theirPawns = FILE_MASKS[file] & position.GetTeamPieces(otherTeam, PIECE_PAWN);

			int ourRank = (ourPawns) ? RelativeRank(team, BitBoard::RankOfIndex(BackmostSquare(ourPawns, team))) : 0;
			int theirRank = (theirPawns) ? RelativeRank(otherTeam, BitBoard::RankOfIndex(FrontmostSquare(theirPawns, otherTeam))) : 0;

			int distance = std::min<int>(file, FILE_MAX - file - 1);
			mg += s_KingShieldStength[distance][ourRank];
			if (ourRank > 0 && ourRank == theirRank - 1)
			{
				if (theirRank == RANK_3)
					mg -= 30;
			}
			else
			{
				mg -= s_PawnStormStrength[distance][theirRank];
			}
		}

		result.KingSafety[MIDGAME][team] = mg;
		result.KingSafety[ENDGAME][team] = 0;
	}

	void EvaluateKingSafety(EvaluationResult& result, const Position& position)
	{
		EvaluateKingSafety<TEAM_WHITE>(result, position);
		EvaluateKingSafety<TEAM_BLACK>(result, position);
	}

	template<Team team>
	void EvaluateSpace(EvaluationResult& result, const Position& position)
	{
		if (position.GetNonPawnMaterial() < 4500)
		{
			result.Space[MIDGAME][team] = 0;
			result.Space[ENDGAME][team] = 0;
			return;
		}
		constexpr Team otherTeam = OtherTeam(team);

		constexpr BitBoard spaceMask =
			(team == TEAM_WHITE) ? (FILE_C_MASK | FILE_D_MASK | FILE_E_MASK | FILE_F_MASK) & (RANK_2_MASK | RANK_3_MASK | RANK_4_MASK)
			: (FILE_C_MASK | FILE_D_MASK | FILE_E_MASK | FILE_F_MASK) & (RANK_7_MASK | RANK_6_MASK | RANK_5_MASK);

		BitBoard safe = spaceMask & ~position.GetTeamPieces(team, PIECE_PAWN) & ~result.Data.AttackedBy[otherTeam][PIECE_PAWN];
		BitBoard behind = position.GetTeamPieces(team, PIECE_PAWN);
		behind |= ((team == TEAM_WHITE) ? behind >> 8 : behind << 8);
		behind |= ((team == TEAM_WHITE) ? behind >> 8 : behind << 8);

		int count = safe.GetCount() + (behind & safe).GetCount();
		int weight = position.GetTeamPieces(team).GetCount();

		result.Space[MIDGAME][team] = count * weight * weight / 16;
		result.Space[ENDGAME][team] = 0;
	}

	void EvaluateSpace(EvaluationResult& result, const Position& position)
	{
		EvaluateSpace<TEAM_WHITE>(result, position);
		EvaluateSpace<TEAM_BLACK>(result, position);
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
		BOX_ASSERT(!IsInCheck(position, position.TeamToPlay), "Cannot evaluate position in check");
		EvaluationResult result;
		ClearResult(result);
		result.GameStage = CalculateGameStage(position);
		EvaluateMaterial(result, position);
		EvaluatePieceSquareTables(result, position);
			
		Centipawns current = result.GetTotal(team);
		constexpr Centipawns delta = 250;
		if (current > alpha - delta && current < beta + delta)
		{
			result.Data.KingAttackZone[TEAM_WHITE] = CalculateKingAttackRegion<TEAM_WHITE>(position);
			result.Data.KingAttackZone[TEAM_BLACK] = CalculateKingAttackRegion<TEAM_BLACK>(position);

			result.Data.AttackedBy[TEAM_WHITE][PIECE_PAWN] = CalculatePawnAttacks<TEAM_WHITE>(position);
			result.Data.AttackedBy[TEAM_BLACK][PIECE_PAWN] = CalculatePawnAttacks<TEAM_BLACK>(position);
			result.Data.AttackedBy[TEAM_WHITE][PIECE_KING] = GetNonSlidingAttacks(PIECE_KING, position.GetKingSquare(TEAM_WHITE), TEAM_WHITE);
			result.Data.AttackedBy[TEAM_BLACK][PIECE_KING] = GetNonSlidingAttacks(PIECE_KING, position.GetKingSquare(TEAM_BLACK), TEAM_BLACK);
			result.Data.AttackedByTwice[TEAM_WHITE] = result.Data.AttackedBy[TEAM_WHITE][PIECE_PAWN] & result.Data.AttackedBy[TEAM_WHITE][PIECE_KING];
			result.Data.AttackedByTwice[TEAM_BLACK] = result.Data.AttackedBy[TEAM_BLACK][PIECE_PAWN] & result.Data.AttackedBy[TEAM_BLACK][PIECE_KING];

			// Do these first as they set AttackedBy data
			EvaluateKnights(result, position);
			EvaluateBishops(result, position);
			EvaluateRooks(result, position);
			EvaluateQueens(result, position);

			EvaluatePassedPawns(result, position);
			EvaluateDoubledPawns(result, position);
			EvaluateWeakPawns(result, position);
			EvaluateBlockedPieces(result, position);
			EvaluateSpace(result, position);
			EvaluateKingSafety(result, position);
			EvaluateTempo(result, position);
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
			evaluation.Space[MIDGAME][TEAM_WHITE] +
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
			evaluation.Space[ENDGAME][TEAM_WHITE] +
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
			evaluation.Space[MIDGAME][TEAM_BLACK] +
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
			evaluation.Space[ENDGAME][TEAM_BLACK] +
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
			evaluation.Space[MIDGAME][TEAM_WHITE] - evaluation.Space[MIDGAME][TEAM_BLACK] +
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
			evaluation.Space[ENDGAME][TEAM_WHITE] - evaluation.Space[ENDGAME][TEAM_BLACK] +
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
		result += "          Space | " + FormatScore(evaluation.Space[MIDGAME][TEAM_WHITE], scoreLength)         + " " + FormatScore(evaluation.Space[ENDGAME][TEAM_WHITE], scoreLength)         + " | " + FormatScore(evaluation.Space[MIDGAME][TEAM_BLACK], scoreLength)         + " " + FormatScore(evaluation.Space[ENDGAME][TEAM_BLACK], scoreLength)         + " | " + FormatScore(evaluation.Space[MIDGAME][TEAM_WHITE] - evaluation.Space[MIDGAME][TEAM_BLACK], scoreLength)                 + " " + FormatScore(evaluation.Space[ENDGAME][TEAM_WHITE] - evaluation.Space[ENDGAME][TEAM_BLACK], scoreLength)                 + '\n';
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
