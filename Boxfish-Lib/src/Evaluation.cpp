#include "Evaluation.h"
#include "MoveGenerator.h"
#include "Attacks.h"

namespace Boxfish
{

	constexpr bool UseMaterial = true;
	constexpr bool UsePieceSquares = true;
	constexpr bool UseBlockedPieces = true;
	constexpr bool UseKingSafety = true;
	constexpr bool UseThreats = true;
	constexpr bool UseSpace = true;
	constexpr bool UseInitiative = true;

	// =================================================================================================================================
	// INITIALIZATION
	// =================================================================================================================================

	static bool s_Initialized = false;

	constexpr BitBoard QueenSide = FILE_A_MASK | FILE_B_MASK | FILE_C_MASK | FILE_D_MASK;
	constexpr BitBoard CenterFiles = FILE_C_MASK | FILE_D_MASK | FILE_E_MASK | FILE_F_MASK;
	constexpr BitBoard CenterRanks = RANK_3_MASK | RANK_4_MASK | RANK_5_MASK | RANK_6_MASK;
	constexpr BitBoard KingSide = FILE_E_MASK | FILE_F_MASK | FILE_G_MASK | FILE_H_MASK;
	constexpr BitBoard Center = (FILE_D_MASK | FILE_E_MASK) & (RANK_4_MASK | RANK_5_MASK);

	static constexpr SquareIndex s_OppositeSquare[SQUARE_MAX] = {
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

	static int s_PhaseWeights[PIECE_MAX];
	int s_MaxPhaseValue = 0;

	static int s_DistanceTable[SQUARE_MAX][SQUARE_MAX];

	static ValueType s_MaterialValues[GAME_STAGE_MAX][PIECE_MAX];
	static ValueType s_PieceSquareTables[GAME_STAGE_MAX][TEAM_MAX][PIECE_MAX][SQUARE_MAX];

	static constexpr ValueType s_KnightAdjust[9] = { -20, -16, -12, -8, -4, 0, 4, 8, 12 };
	static constexpr ValueType s_RookAdjust[9] = { 15, 12, 9, 6, 3, 0, -3, -6, -9 };

	static BitBoard s_PawnShieldMasks[TEAM_MAX][SQUARE_MAX];
	static BitBoard s_PassedPawnMasks[TEAM_MAX][SQUARE_MAX];
	static ValueType s_PassedPawnRankWeights[GAME_STAGE_MAX][TEAM_MAX][RANK_MAX];
	static BitBoard s_SupportedPawnMasks[TEAM_MAX][SQUARE_MAX];

	static constexpr BitBoard s_KingFlanks[FILE_MAX] = {
		QueenSide & ~FILE_D_MASK,
		QueenSide,
		QueenSide,
		CenterFiles,
		CenterFiles,
		KingSide,
		KingSide,
		KingSide & ~FILE_E_MASK
	};

	static constexpr ValueType s_MobilityWeights[GAME_STAGE_MAX][PIECE_MAX] = {
		{ 0, 4, 3, 2, 1, 0 },
		{ 0, 4, 3, 1, 2, 0 },
	};

	static constexpr int s_MobilityOffsets[PIECE_MAX] = {
		0, 3, 6, 6, 12, 0,
	};

	static constexpr ValueType s_TropismWeights[GAME_STAGE_MAX][PIECE_MAX] = {
		{ 0, 3, 2, 2, 2, 0 },
		{ 0, 3, 1, 1, 4, 0 },
	};

	// Indexed by attack units
	static constexpr int MAX_ATTACK_UNITS = 100;
	static constexpr ValueType s_KingSafetyTable[MAX_ATTACK_UNITS] = {
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
	static constexpr ValueType s_KingShieldStength[FILE_MAX / 2][RANK_MAX] = {
		{ -3, 40, 45, 26, 20, 9, 13 },
		{ -21, 22, 17, -25, -14, -5, -32 },
		{ -5, 38, 12, -1, 16, 1, -22 },
		{ -20, -7, -14, -15, -24, -33, -88 },
	};

	// Pawns can never be on the 8th rank (1st rank used as a sentinel for when there is no pawn)
	static constexpr ValueType s_PawnStormStrength[FILE_MAX / 2][RANK_MAX] = {
		{ 45, -145, -85, 48, 27, 23, 27 },
		{ 22, -13, 62, 23, 18, -3, 11 },
		{ -3, 25, 81, 17, -1, -10, -5 },
		{ -8, -6, 50, 2, 5, -8, -11 },
	};

	static constexpr ValueType s_BlockedStormStrength[RANK_MAX] = {
		0, 0, 36, -5, -4, -2, -1
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

	void MirrorTable(ValueType* dest, ValueType* src)
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
		s_PhaseWeights[PIECE_ROOK] = 3;
		s_PhaseWeights[PIECE_QUEEN] = 6;
		s_PhaseWeights[PIECE_KING] = 0;

		s_MaxPhaseValue = s_PhaseWeights[PIECE_PAWN] * 16 + s_PhaseWeights[PIECE_KNIGHT] * 4 + s_PhaseWeights[PIECE_BISHOP] * 4 + s_PhaseWeights[PIECE_ROOK] * 4 + s_PhaseWeights[PIECE_QUEEN] * 2 + s_PhaseWeights[PIECE_KING] * 2;
	}

	void InitPawnShieldMasks()
	{
		for (SquareIndex square = a1; square < SQUARE_MAX; square++)
		{
			s_PawnShieldMasks[TEAM_WHITE][square] = ((square << 8) | ((square << 7) & ~FILE_H_MASK) | ((square << 9) & ~FILE_A_MASK)) & (RANK_2_MASK | RANK_3_MASK);
			s_PawnShieldMasks[TEAM_BLACK][square] = ((square >> 8) | ((square >> 7) & ~FILE_H_MASK) | ((square >> 9) & ~FILE_A_MASK)) & (RANK_7_MASK | RANK_6_MASK);
		}
	}

	void InitPassedPawnMasks()
	{
		for (SquareIndex square = a1; square < SQUARE_MAX; square++)
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
		// For now only pawns can have different endgame value
		s_MaterialValues[MIDGAME][PIECE_PAWN] = 100;
		s_MaterialValues[MIDGAME][PIECE_KNIGHT] = 350;
		s_MaterialValues[MIDGAME][PIECE_BISHOP] = 375;
		s_MaterialValues[MIDGAME][PIECE_ROOK] = 515;
		s_MaterialValues[MIDGAME][PIECE_QUEEN] = 975;
		s_MaterialValues[MIDGAME][PIECE_KING] = 20000;

		s_MaterialValues[ENDGAME][PIECE_PAWN] = 140;
		s_MaterialValues[ENDGAME][PIECE_KNIGHT] = 350;
		s_MaterialValues[ENDGAME][PIECE_BISHOP] = 375;
		s_MaterialValues[ENDGAME][PIECE_ROOK] = 515;
		s_MaterialValues[ENDGAME][PIECE_QUEEN] = 975;
		s_MaterialValues[ENDGAME][PIECE_KING] = 20000;
	}

	void InitPieceSquareTables()
	{
		ValueType whitePawnsTable[SQUARE_MAX] = {
			 0,  0,  0,  0,  0,  0,  0,  0,
			50, 50, 50, 50, 50, 50, 50, 50,
			10, 10, 20, 20, 30, 20, 10, 10,
			 5,  5, 10,  5, 10, 10,  5,  5,
			 5,  0, 10, 15, 25,  0,  0,  5,
			 5,  5,  5,  7, 15, 11,  5,  5,
			 5, 10, 10,-10,-10, 10,  5,  5,
			 0,  0,  0,  0,  0,  0,  0,  0
		};
		MirrorTable(s_PieceSquareTables[MIDGAME][TEAM_WHITE][PIECE_PAWN], whitePawnsTable);
		MirrorTable(s_PieceSquareTables[MIDGAME][TEAM_BLACK][PIECE_PAWN], s_PieceSquareTables[MIDGAME][TEAM_WHITE][PIECE_PAWN]);
		MirrorTable(s_PieceSquareTables[ENDGAME][TEAM_WHITE][PIECE_PAWN], s_PieceSquareTables[MIDGAME][TEAM_BLACK][PIECE_PAWN]);
		MirrorTable(s_PieceSquareTables[ENDGAME][TEAM_BLACK][PIECE_PAWN], s_PieceSquareTables[MIDGAME][TEAM_WHITE][PIECE_PAWN]);

		ValueType whiteKnightsTable[SQUARE_MAX] = {
			-50,-40,-30,-30,-30,-30,-40,-50,
			-40,  0,  0,  0,  0,  0,  0,-40,
			-30,  0, 28, 25, 25, 28,  0,-30,
			-30,  5, 15, 20, 20, 15,  5,-30,
			-30,  0, 15, 20, 20, 15,  0,-30,
			-30,  5, 10, 15, 15, 10,  5,-30,
			-40,  0,  0,  5,  5,  0,  0,-40,
			-50,-40,-30,-30,-30,-30,-40,-50,
		};
		MirrorTable(s_PieceSquareTables[MIDGAME][TEAM_WHITE][PIECE_KNIGHT], whiteKnightsTable);
		MirrorTable(s_PieceSquareTables[MIDGAME][TEAM_BLACK][PIECE_KNIGHT], s_PieceSquareTables[MIDGAME][TEAM_WHITE][PIECE_KNIGHT]);
		MirrorTable(s_PieceSquareTables[ENDGAME][TEAM_WHITE][PIECE_KNIGHT], s_PieceSquareTables[MIDGAME][TEAM_BLACK][PIECE_KNIGHT]);
		MirrorTable(s_PieceSquareTables[ENDGAME][TEAM_BLACK][PIECE_KNIGHT], s_PieceSquareTables[MIDGAME][TEAM_WHITE][PIECE_KNIGHT]);

		ValueType whiteBishopsTable[SQUARE_MAX] = {
			-20,-10,-10,-10,-10,-10,-10,-20,
			-10,  0,  0,  0,  0,  0,  0,-10,
			-10,  0,  5, 10, 10,  5,  0,-10,
			-10,  5,  5, 15, 15,  5,  5,-10,
			-10,  0, 10, 25, 25, 10,  0,-10,
			-10, 10, 10, 10, 10, 10, 10,-10,
			-10, 15, 10,  5,  5, 10, 15,-10,
			-20,-10,-10,-10,-10,-10,-10,-20,
		};
		MirrorTable(s_PieceSquareTables[MIDGAME][TEAM_WHITE][PIECE_BISHOP], whiteBishopsTable);
		MirrorTable(s_PieceSquareTables[MIDGAME][TEAM_BLACK][PIECE_BISHOP], s_PieceSquareTables[MIDGAME][TEAM_WHITE][PIECE_BISHOP]);
		MirrorTable(s_PieceSquareTables[ENDGAME][TEAM_WHITE][PIECE_BISHOP], s_PieceSquareTables[MIDGAME][TEAM_BLACK][PIECE_BISHOP]);
		MirrorTable(s_PieceSquareTables[ENDGAME][TEAM_BLACK][PIECE_BISHOP], s_PieceSquareTables[MIDGAME][TEAM_WHITE][PIECE_BISHOP]);

		ValueType whiteRooksTable[SQUARE_MAX] = {
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

		ValueType whiteQueensTable[SQUARE_MAX] = {
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

		ValueType whiteKingsTableMidgame[SQUARE_MAX] = {
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

		ValueType whiteKingsTableEndgame[SQUARE_MAX] = {
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
		for (SquareIndex i = a1; i < SQUARE_MAX; i++)
		{
			for (SquareIndex j = a1; j < SQUARE_MAX; j++)
			{
				Square sqi = BitBoard::BitIndexToSquare(i);
				Square sqj = BitBoard::BitIndexToSquare(j);
				s_DistanceTable[i][j] = std::abs(sqi.File - sqj.File) + std::abs(sqi.Rank - sqj.Rank);
			}
		}
	}

	void InitSafetyTables()
	{
		s_AttackUnits[PIECE_PAWN] = 0;
		s_AttackUnits[PIECE_KNIGHT] = 8;
		s_AttackUnits[PIECE_BISHOP] = 3;
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

	template<Team TEAM>
	BitBoard CalculateKingAttackRegion(const Position& position)
	{
		Square sq = BitBoard::BitIndexToSquare(position.GetKingSquare(TEAM));
		SquareIndex square = BitBoard::SquareToBitIndex({ std::clamp(sq.File, FILE_B, FILE_G), std::clamp(sq.Rank, RANK_2, RANK_7) });
		return GetNonSlidingAttacks<PIECE_KING>(square) | square;
	}

	bool IsPawnSupported(const Position& position, SquareIndex square, Team team)
	{
		return (s_SupportedPawnMasks[team][square] & position.GetTeamPieces(team, PIECE_PAWN)) != ZERO_BB;
	}

	inline int CalculateTropism(SquareIndex a, SquareIndex b)
	{
		return 14 - s_DistanceTable[a][b];
	}

	template<Team TEAM>
	constexpr BitBoard GetPawnDoubleAttacks(BitBoard pawns)
	{
		return (TEAM == TEAM_WHITE) ? Shift<NORTH_WEST>(pawns) & Shift<NORTH_EAST>(pawns)
									: Shift<SOUTH_EAST>(pawns) & Shift<SOUTH_WEST>(pawns);
	}

	// =================================================================================================================================
	// EVALUATION FUNCTIONS
	// =================================================================================================================================

	template<Team TEAM>
	int EvaluateMaterial(EvaluationResult& result, const Position& position)
	{
		if constexpr (UseMaterial)
		{
			int pawnCount = position.GetTeamPieces(TEAM, PIECE_PAWN).GetCount();
			result.Material[MIDGAME][TEAM] = pawnCount * s_MaterialValues[MIDGAME][PIECE_PAWN] + position.GetNonPawnMaterial(TEAM);
			result.Material[ENDGAME][TEAM] = pawnCount * s_MaterialValues[ENDGAME][PIECE_PAWN] + position.GetNonPawnMaterial(TEAM);
			return pawnCount;
		}
		else
		{
			result.Material[MIDGAME][TEAM] = 0;
			result.Material[ENDGAME][TEAM] = 0;
			return 1;
		}
	}

	void EvaluateMaterial(EvaluationResult& result, const Position& position)
	{
		int whitePawnCount = EvaluateMaterial<TEAM_WHITE>(result, position);
		int blackPawnCount = EvaluateMaterial<TEAM_BLACK>(result, position);

		// Draw if only 1 minor piece
		if (result.Material[ENDGAME][TEAM_WHITE] > result.Material[ENDGAME][TEAM_BLACK] && whitePawnCount == 0)
		{
			result.IsDraw = position.GetNonPawnMaterial(TEAM_WHITE) < GetPieceValue(PIECE_ROOK, MIDGAME);
		}
		else if (result.Material[ENDGAME][TEAM_BLACK] > result.Material[ENDGAME][TEAM_WHITE] && blackPawnCount == 0)
		{
			result.IsDraw = position.GetNonPawnMaterial(TEAM_BLACK) < GetPieceValue(PIECE_ROOK, MIDGAME);
		}
		// King vs King
		else if (result.Material[ENDGAME][TEAM_WHITE] == 0 && result.Material[ENDGAME][TEAM_BLACK] == 0)
		{
			result.IsDraw = true;
		}
	}

	template<Team TEAM>
	void EvaluatePieceSquareTables(EvaluationResult& result, const Position& position)
	{
		if constexpr (UsePieceSquares)
		{
			ValueType mg = 0;
			ValueType eg = 0;
			// Other pieces handled by piece evaluation
			for (Piece piece : { PIECE_PAWN, PIECE_KING })
			{
				BitBoard pieces = position.GetTeamPieces(TEAM, piece);
				while (pieces)
				{
					SquareIndex square = PopLeastSignificantBit(pieces);
					mg += s_PieceSquareTables[MIDGAME][TEAM][piece][square];
					eg += s_PieceSquareTables[ENDGAME][TEAM][piece][square];
				}
			}
			result.PieceSquares[MIDGAME][TEAM] = mg;
			result.PieceSquares[ENDGAME][TEAM] = eg;
		}
		else
		{
			result.PieceSquares[MIDGAME][TEAM] = 0;
			result.PieceSquares[ENDGAME][TEAM] = 0;
		}
	}

	void EvaluatePieceSquareTables(EvaluationResult& result, const Position& position)
	{
		EvaluatePieceSquareTables<TEAM_WHITE>(result, position);
		EvaluatePieceSquareTables<TEAM_BLACK>(result, position);
	}

	template<Team TEAM>
	void EvaluateBlockedPieces(EvaluationResult& result, const Position& position)
	{
		if constexpr (UseBlockedPieces)
		{
			constexpr Team OTHER_TEAM = OtherTeam(TEAM);
			ValueType mg = 0;
			ValueType eg = 0;

			constexpr ValueType TRAPPED_BISHOP_A8[GAME_STAGE_MAX] = { -150, -50 };
			constexpr ValueType TRAPPED_BISHOP_A7[GAME_STAGE_MAX] = { -100, -50 };
			constexpr ValueType TRAPPED_BISHOP_A6[GAME_STAGE_MAX] = { -50, -50 };
			constexpr ValueType TRAPPED_BISHOP_B8[GAME_STAGE_MAX] = { -150, -50 };

			constexpr ValueType TRAPPED_ROOK[GAME_STAGE_MAX] = { -25, -25 };

			// Trapped Bishops
			if (position.IsPieceOnSquare(TEAM, PIECE_BISHOP, RelativeSquare(TEAM, a7)) && position.IsPieceOnSquare(OTHER_TEAM, PIECE_PAWN, RelativeSquare(TEAM, b6)))
			{
				mg += TRAPPED_BISHOP_A7[MIDGAME];
				eg += TRAPPED_BISHOP_A7[ENDGAME];
			}
			if (position.IsPieceOnSquare(TEAM, PIECE_BISHOP, RelativeSquare(TEAM, h7)) && position.IsPieceOnSquare(OTHER_TEAM, PIECE_PAWN, RelativeSquare(TEAM, g6)))
			{
				mg += TRAPPED_BISHOP_A7[MIDGAME];
				eg += TRAPPED_BISHOP_A7[ENDGAME];
			}
			if (position.IsPieceOnSquare(TEAM, PIECE_BISHOP, RelativeSquare(TEAM, a6)) && position.IsPieceOnSquare(OTHER_TEAM, PIECE_PAWN, RelativeSquare(TEAM, b5)))
			{
				mg += TRAPPED_BISHOP_A6[MIDGAME];
				eg += TRAPPED_BISHOP_A6[ENDGAME];
			}
			if (position.IsPieceOnSquare(TEAM, PIECE_BISHOP, RelativeSquare(TEAM, h6)) && position.IsPieceOnSquare(OTHER_TEAM, PIECE_PAWN, RelativeSquare(TEAM, g5)))
			{
				mg += TRAPPED_BISHOP_A6[MIDGAME];
				eg += TRAPPED_BISHOP_A6[ENDGAME];
			}
			if (position.IsPieceOnSquare(TEAM, PIECE_BISHOP, RelativeSquare(TEAM, b8)) && position.IsPieceOnSquare(OTHER_TEAM, PIECE_PAWN, RelativeSquare(TEAM, c7)))
			{
				mg += TRAPPED_BISHOP_B8[MIDGAME];
				eg += TRAPPED_BISHOP_B8[ENDGAME];
			}
			if (position.IsPieceOnSquare(TEAM, PIECE_BISHOP, RelativeSquare(TEAM, g8)) && position.IsPieceOnSquare(OTHER_TEAM, PIECE_PAWN, RelativeSquare(TEAM, f7)))
			{
				mg += TRAPPED_BISHOP_B8[MIDGAME];
				eg += TRAPPED_BISHOP_B8[ENDGAME];
			}
			if (position.IsPieceOnSquare(TEAM, PIECE_BISHOP, RelativeSquare(TEAM, a8)) && position.IsPieceOnSquare(OTHER_TEAM, PIECE_PAWN, RelativeSquare(TEAM, b7)))
			{
				mg += TRAPPED_BISHOP_A8[MIDGAME];
				eg += TRAPPED_BISHOP_A8[ENDGAME];
			}
			if (position.IsPieceOnSquare(TEAM, PIECE_BISHOP, RelativeSquare(TEAM, h8)) && position.IsPieceOnSquare(OTHER_TEAM, PIECE_PAWN, RelativeSquare(TEAM, g7)))
			{
				mg += TRAPPED_BISHOP_A8[MIDGAME];
				eg += TRAPPED_BISHOP_A8[ENDGAME];
			}

			// Blocked Rook
			if ((position.IsPieceOnSquare(TEAM, PIECE_ROOK, RelativeSquare(TEAM, g1)) || position.IsPieceOnSquare(TEAM, PIECE_ROOK, RelativeSquare(TEAM, h1)))
				&& (position.IsPieceOnSquare(TEAM, PIECE_KING, RelativeSquare(TEAM, g1)) || position.IsPieceOnSquare(TEAM, PIECE_KING, RelativeSquare(TEAM, f1))))
			{
				mg += TRAPPED_ROOK[MIDGAME];
				eg += TRAPPED_ROOK[ENDGAME];
			}
			else if ((position.IsPieceOnSquare(TEAM, PIECE_ROOK, RelativeSquare(TEAM, a1)) || position.IsPieceOnSquare(TEAM, PIECE_ROOK, RelativeSquare(TEAM, b1)))
				&& (position.IsPieceOnSquare(TEAM, PIECE_KING, RelativeSquare(TEAM, b1)) || position.IsPieceOnSquare(TEAM, PIECE_KING, RelativeSquare(TEAM, c1))))
			{
				mg += TRAPPED_ROOK[MIDGAME];
				eg += TRAPPED_ROOK[ENDGAME];
			}

			result.BlockedPieces[MIDGAME][TEAM] = mg;
			result.BlockedPieces[ENDGAME][TEAM] = eg;
		}
		else
		{
			result.BlockedPieces[MIDGAME][TEAM] = 0;
			result.BlockedPieces[ENDGAME][TEAM] = 0;
		}
	}

	void EvaluateBlockedPieces(EvaluationResult& result, const Position& position)
	{
		EvaluateBlockedPieces<TEAM_WHITE>(result, position);
		EvaluateBlockedPieces<TEAM_BLACK>(result, position);
	}

	template<Team TEAM>
	void EvaluatePawns(EvaluationResult& result, const Position& position)
	{
		constexpr Team OTHER_TEAM = OtherTeam(TEAM);
		constexpr ValueType SupportedPassedPawn = 35;

		ValueType mg = 0;
		ValueType eg = 0;

		// Passed pawns
		BitBoard pawns = position.GetTeamPieces(TEAM, PIECE_PAWN);
		BitBoard otherPawns = position.GetTeamPieces(OTHER_TEAM, PIECE_PAWN);
		while (pawns)
		{
			SquareIndex square = PopLeastSignificantBit(pawns);
			const bool isSupported = IsPawnSupported(position, square, TEAM);
			if ((otherPawns & s_PassedPawnMasks[TEAM][square]) == ZERO_BB)
			{
				Rank rank = BitBoard::RankOfIndex(square);
				mg += s_PassedPawnRankWeights[MIDGAME][TEAM][rank];
				eg += s_PassedPawnRankWeights[ENDGAME][TEAM][rank];
				if (isSupported)
				{
					mg += SupportedPassedPawn * 1;
					eg += SupportedPassedPawn * 2;
				}
			}
		}

		// Doubled pawns
		int doubledCount = 0;
		for (File file = FILE_A; file < FILE_MAX; file++)
		{
			int pawnsOnFile = (position.GetTeamPieces(TEAM, PIECE_PAWN) & FILE_MASKS[file]).GetCount();
			if (pawnsOnFile > 1)
				doubledCount += pawnsOnFile - 1;
		}
		mg -= 5 * doubledCount;
		eg -= 30 * doubledCount;

		result.Pawns[MIDGAME][TEAM] = mg;
		result.Pawns[ENDGAME][TEAM] = eg;
	}

	void EvaluatePawns(EvaluationResult& result, const Position& position)
	{
		EvaluatePawns<TEAM_WHITE>(result, position);
		EvaluatePawns<TEAM_BLACK>(result, position);
	}

	template<Team TEAM, Piece PIECE>
	void EvaluatePieces(EvaluationResult& result, const Position& position)
	{
		static_assert(PIECE == PIECE_KNIGHT || PIECE == PIECE_BISHOP || PIECE == PIECE_ROOK || PIECE == PIECE_QUEEN);
		constexpr Team OTHER_TEAM = OtherTeam(TEAM);
		constexpr BitBoard OutpostZone =
			(TEAM == TEAM_WHITE ? (RANK_4_MASK | RANK_5_MASK | RANK_6_MASK)
							    : (RANK_5_MASK | RANK_4_MASK | RANK_3_MASK)) & CenterFiles;
		constexpr Direction Down = TEAM == TEAM_WHITE ? SOUTH : NORTH;

		ValueType mg = 0;
		ValueType eg = 0;

		result.Data.AttackedBy[TEAM][PIECE] = ZERO_BB;

		BitBoard outpostSquares = result.Data.AttackedBy[TEAM][PIECE_PAWN];
		if constexpr (PIECE == PIECE_KNIGHT || PIECE == PIECE_BISHOP)
		{
			outpostSquares &= OutpostZone;
			BitBoard b = outpostSquares;
			while (b)
			{
				SquareIndex square = PopLeastSignificantBit(b);
				File file = BitBoard::FileOfIndex(square);
				Rank rank = BitBoard::RankOfIndex(square);
				if (result.Data.AttackedBy[OTHER_TEAM][PIECE_PAWN] & (FILE_MASKS[file] & InFrontOrEqual(rank, TEAM)))
					outpostSquares &= ~FILE_MASKS[file];
			}
		}

		BitBoard pieceSquares = position.GetTeamPieces(TEAM, PIECE);
		while (pieceSquares)
		{
			SquareIndex square = PopLeastSignificantBit(pieceSquares);

			if constexpr (UsePieceSquares)
			{
				result.PieceSquares[MIDGAME][TEAM] += s_PieceSquareTables[MIDGAME][TEAM][PIECE][square];
				result.PieceSquares[ENDGAME][TEAM] += s_PieceSquareTables[ENDGAME][TEAM][PIECE][square];
			}

			BitBoard attacks =
				PIECE == PIECE_BISHOP	? GetSlidingAttacks<PIECE_BISHOP>(square, position.GetAllPieces() ^ position.GetPieces(PIECE_QUEEN)) :
				PIECE == PIECE_ROOK		? GetSlidingAttacks<PIECE_ROOK>(square, position.GetAllPieces() ^ position.GetPieces(PIECE_QUEEN) ^ position.GetTeamPieces(TEAM, PIECE_ROOK)) :
				PIECE == PIECE_QUEEN	? GetSlidingAttacks<PIECE_QUEEN>(square, position.GetAllPieces()) :
				PIECE == PIECE_KNIGHT	? GetNonSlidingAttacks<PIECE_KNIGHT>(square) : ZERO_BB;

			if (position.GetBlockersForKing(TEAM) & square)
				attacks &= GetLineBetween(position.GetKingSquare(TEAM), square);

			result.Data.AttackedByTwice[TEAM] |= result.Data.AttackedBy[TEAM][PIECE_ALL] & attacks;
			result.Data.AttackedBy[TEAM][PIECE] |= attacks;
			result.Data.AttackedBy[TEAM][PIECE_ALL] |= attacks;

			BitBoard attacksOnKing = attacks & result.Data.KingAttackZone[OTHER_TEAM];
			if (attacksOnKing != ZERO_BB)
			{
				result.Data.Attackers[TEAM]++;
				result.Data.AttackUnits[TEAM] += s_AttackUnits[PIECE] * (attacks & result.Data.AttackedBy[OTHER_TEAM][PIECE_KING]).GetCount();
			}
			// Bishop and Rook X-raying king attack zone
			else if (PIECE == PIECE_BISHOP && (GetSlidingAttacks<PIECE_BISHOP>(square, position.GetPieces(PIECE_PAWN)) & result.Data.KingAttackZone[OTHER_TEAM]))
				mg += 15;
			else if (PIECE == PIECE_ROOK && (FILE_MASKS[BitBoard::FileOfIndex(square)] & result.Data.KingAttackZone[OTHER_TEAM]))
				mg += 5;

			int reachableSquares = (attacks & ~result.Data.AttackedBy[OTHER_TEAM][PIECE_PAWN]).GetCount();
			mg += s_MobilityWeights[MIDGAME][PIECE] * (reachableSquares - s_MobilityOffsets[PIECE]);
			eg += s_MobilityWeights[ENDGAME][PIECE] * (reachableSquares - s_MobilityOffsets[PIECE]);

			//mg += s_TropismWeights[MIDGAME][PIECE] * CalculateTropism(position.GetKingSquare(OTHER_TEAM), square);
			//eg += s_TropismWeights[ENDGAME][PIECE] * CalculateTropism(position.GetKingSquare(OTHER_TEAM), square);

			if constexpr (PIECE == PIECE_KNIGHT || PIECE == PIECE_BISHOP)
			{
				File file = BitBoard::FileOfIndex(square);
				Rank rank = BitBoard::RankOfIndex(square);
				
				constexpr int OutpostBonus = 30;
				// Knight supported by a pawn and no enemy pawns on neighbour files - Outpost
				if (outpostSquares & square)
				{
					mg += OutpostBonus;
					eg += OutpostBonus / 2;
				}
				else if (outpostSquares & attacks)
				{
					mg += OutpostBonus / 2;
					eg += OutpostBonus / 4;
				}
				int distance = s_DistanceTable[square][position.GetKingSquare(TEAM)];
				mg -= distance * (4 - (PIECE == PIECE_BISHOP));

				// Minor piece behind pawn
				if (Shift<Down>(position.GetPieces(PIECE_PAWN)) & square)
				{
					mg += 10;
				}
			}
			if constexpr (PIECE == PIECE_BISHOP)
			{
				if (MoreThanOne(GetSlidingAttacks<PIECE_BISHOP>(square, position.GetPieces(PIECE_PAWN)) & Center))
					mg += 30;
			}
			if constexpr (PIECE == PIECE_ROOK)
			{
				int pawnCountOnFile = (FILE_MASKS[BitBoard::FileOfIndex(square)] & position.GetPieces(PIECE_PAWN)).GetCount();
				mg += 12 * (2 - pawnCountOnFile);
				eg += 5 * (2 - pawnCountOnFile);
			}
			if constexpr (PIECE == PIECE_QUEEN)
			{
				if (GetSliderBlockers(position, position.GetTeamPieces(OTHER_TEAM, PIECE_ROOK, PIECE_BISHOP), square, nullptr))
				{
					mg -= 25;
					eg -= 5;
				}
			}
		}

		result.SetPieceEval<TEAM, PIECE>(mg, eg);
	}

	template<Piece PIECE>
	void EvaluatePieces(EvaluationResult& result, const Position& position)
	{
		EvaluatePieces<TEAM_WHITE, PIECE>(result, position);
		EvaluatePieces<TEAM_BLACK, PIECE>(result, position);
	}

	template<Team TEAM>
	void EvaluateKingSafety(EvaluationResult& result, const Position& position)
	{
		if constexpr (UseKingSafety)
		{
			constexpr Team OTHER_TEAM = OtherTeam(TEAM);
			constexpr BitBoard OurSide =
				(TEAM == TEAM_WHITE) ? ALL_SQUARES_BB & ~RANK_6_MASK & ~RANK_7_MASK & ~RANK_8_MASK
				: ALL_SQUARES_BB & ~RANK_1_MASK & ~RANK_2_MASK & ~RANK_3_MASK;

			SquareIndex kngSq = position.GetKingSquare(TEAM);
			Square kingSquare = BitBoard::BitIndexToSquare(kngSq);

			ValueType mg = 0;
			ValueType eg = 0;
			if (result.Data.Attackers[OTHER_TEAM] >= 2 && position.GetTeamPieces(OTHER_TEAM, PIECE_QUEEN) != ZERO_BB)
			{
				mg -= s_KingSafetyTable[std::min(result.Data.AttackUnits[OTHER_TEAM], MAX_ATTACK_UNITS - 1)];
			}

			const BitBoard pawnMask = InFrontOrEqual(kingSquare.Rank, TEAM);

			// King Shield
			File kingFileCenter = std::clamp(kingSquare.File, FILE_B, FILE_G);
			for (File file = (File)(kingFileCenter - 1); file <= kingFileCenter + 1; file++)
			{
				// Only include pawns in front of our king
				BitBoard ourPawns = FILE_MASKS[file] & position.GetTeamPieces(TEAM, PIECE_PAWN) & pawnMask & ~result.Data.AttackedBy[OTHER_TEAM][PIECE_PAWN];
				BitBoard theirPawns = FILE_MASKS[file] & position.GetTeamPieces(OTHER_TEAM, PIECE_PAWN) & pawnMask;

				int ourRank = (ourPawns) ? RelativeRank(TEAM, BitBoard::RankOfIndex(FrontmostSquare(ourPawns, OTHER_TEAM))) : 0;
				int theirRank = (theirPawns) ? RelativeRank(TEAM, BitBoard::RankOfIndex(FrontmostSquare(theirPawns, OTHER_TEAM))) : 0;

				int distance = std::min<int>(file, FILE_MAX - file - 1);
				mg += s_KingShieldStength[distance][ourRank];
				if (ourRank > 0 && ourRank == theirRank - 1)
				{
					mg -= s_BlockedStormStrength[theirRank];
				}
				else
				{
					mg -= s_PawnStormStrength[distance][theirRank];
				}
			}

			mg -= 3 * (position.GetBlockersForKing(TEAM)).GetCount();
			mg += 20 * (bool)(result.Data.AttackedBy[TEAM][PIECE_KNIGHT] & result.Data.AttackedBy[TEAM][PIECE_KING]);

			BitBoard pawns = position.GetTeamPieces(TEAM, PIECE_PAWN);
			int minDistance = 6;

			if (pawns & GetNonSlidingAttacks<PIECE_KING>(kngSq))
			{
				minDistance = 1;
			}
			else
			{
				while (pawns)
				{
					SquareIndex sq = PopLeastSignificantBit(pawns);
					minDistance = std::min(minDistance, s_DistanceTable[kngSq][sq]);
				}
			}

			eg -= 8 * minDistance;

			result.KingSafety[MIDGAME][TEAM] = mg;
			result.KingSafety[ENDGAME][TEAM] = eg;
		}
		else
		{
			result.KingSafety[MIDGAME][TEAM] = 0;
			result.KingSafety[ENDGAME][TEAM] = 0;
		}
	}

	void EvaluateKingSafety(EvaluationResult& result, const Position& position)
	{
		EvaluateKingSafety<TEAM_WHITE>(result, position);
		EvaluateKingSafety<TEAM_BLACK>(result, position);
	}

	template<Team TEAM>
	void EvaluateThreats(EvaluationResult& result, const Position& position)
	{
		if constexpr (UseThreats)
		{
			constexpr Team OTHER_TEAM = OtherTeam(TEAM);
			constexpr Direction Up = (TEAM == TEAM_WHITE) ? NORTH : SOUTH;
			constexpr BitBoard RelativeRank3BB = (TEAM == TEAM_WHITE) ? RANK_3_MASK : RANK_6_MASK;

			ValueType mg = 0;
			ValueType eg = 0;

			BitBoard stronglyDefended = result.Data.AttackedBy[OTHER_TEAM][PIECE_PAWN] | result.Data.AttackedByTwice[OTHER_TEAM];
			BitBoard safe = ~result.Data.AttackedBy[OTHER_TEAM][PIECE_ALL] | result.Data.AttackedBy[TEAM][PIECE_ALL];
			BitBoard nonPawnEnemies = position.GetTeamPieces(OTHER_TEAM) & ~position.GetTeamPieces(OTHER_TEAM, PIECE_PAWN);

			BitBoard enemyMinors = position.GetTeamPieces(OTHER_TEAM, PIECE_BISHOP, PIECE_KNIGHT);

			BitBoard safePawns = position.GetTeamPieces(TEAM, PIECE_PAWN) & safe;
			BitBoard pawnsOnMinors = GetPawnAttacks<TEAM>(safePawns) & result.Data.AttackedBy[TEAM][PIECE_PAWN] & enemyMinors;
			int pawnThreats = pawnsOnMinors.GetCount();
			mg += 70 * pawnThreats;
			eg += 50 * pawnThreats;

			BitBoard pushedPawns = position.GetTeamPieces(TEAM, PIECE_PAWN) & safe;
			pushedPawns = Shift<Up>(pushedPawns) & ~position.GetAllPieces();
			pushedPawns |= Shift<Up>(pushedPawns & RelativeRank3BB) & ~position.GetAllPieces();
			pushedPawns &= ~result.Data.AttackedBy[OTHER_TEAM][PIECE_PAWN] & safe;
			BitBoard pushedPawnAttacks = GetPawnAttacks<TEAM>(pushedPawns) & nonPawnEnemies;
			int threatenedByPushedPawns = pushedPawnAttacks.GetCount();
			mg += 30 * threatenedByPushedPawns;
			eg += 10 * threatenedByPushedPawns;

			BitBoard minorAttacks = (result.Data.AttackedBy[TEAM][PIECE_BISHOP] | result.Data.AttackedBy[TEAM][PIECE_KNIGHT]);
			BitBoard threatenedMinors = nonPawnEnemies & ~result.Data.AttackedBy[OTHER_TEAM][PIECE_PAWN];
			int minorThreats = (minorAttacks & threatenedMinors).GetCount();
			mg += 45 * minorThreats;
			eg += 15 * minorThreats;

			// Pins
			BitBoard bishopAttacks = result.Data.AttackedBy[TEAM][PIECE_BISHOP];
			BitBoard pinnedPieces = position.GetBlockersForKing(OTHER_TEAM) & nonPawnEnemies & bishopAttacks;
			int pinnedToKing = pinnedPieces.GetCount();
			mg += 25 * pinnedToKing;
			eg += 30 * pinnedToKing;

			BitBoard otherTeamMoves = result.Data.AttackedBy[OTHER_TEAM][PIECE_ALL] & ~stronglyDefended & result.Data.AttackedBy[TEAM][PIECE_ALL];
			int moveCount = otherTeamMoves.GetCount();
			mg += 3 * moveCount;
			eg += 3 * moveCount;

			result.Threats[MIDGAME][TEAM] = mg;
			result.Threats[ENDGAME][TEAM] = eg;
		}
		else
		{
			result.Threats[MIDGAME][TEAM] = 0;
			result.Threats[ENDGAME][TEAM] = 0;
		}
	}

	void EvaluateThreats(EvaluationResult& result, const Position& position)
	{
		EvaluateThreats<TEAM_WHITE>(result, position);
		EvaluateThreats<TEAM_BLACK>(result, position);
	}

	template<Team TEAM>
	void EvaluateSpace(EvaluationResult& result, const Position& position, int blockedPawns)
	{
		if constexpr (UseSpace)
		{
			if (position.GetNonPawnMaterial() < 6250)
			{
				result.Space[MIDGAME][TEAM] = 0;
				result.Space[ENDGAME][TEAM] = 0;
				return;
			}

			constexpr Team OTHER_TEAM = OtherTeam(TEAM);
			constexpr Direction Down = (TEAM == TEAM_WHITE) ? SOUTH : NORTH;
			constexpr BitBoard SpaceMask =
				(TEAM == TEAM_WHITE) ? (FILE_C_MASK | FILE_D_MASK | FILE_E_MASK | FILE_F_MASK) & (RANK_2_MASK | RANK_3_MASK | RANK_4_MASK)
									 : (FILE_C_MASK | FILE_D_MASK | FILE_E_MASK | FILE_F_MASK) & (RANK_7_MASK | RANK_6_MASK | RANK_5_MASK);

			BitBoard safe = SpaceMask & ~position.GetTeamPieces(TEAM, PIECE_PAWN) & ~result.Data.AttackedBy[OTHER_TEAM][PIECE_PAWN];
			BitBoard behind = position.GetTeamPieces(TEAM, PIECE_PAWN);
			behind |= Shift<Down>(behind);
			behind |= Shift<Down>(behind);
			behind |= Shift<Down>(behind);

			int count = safe.GetCount() + (behind & safe & ~result.Data.AttackedBy[OTHER_TEAM][PIECE_ALL]).GetCount();
			int weight = position.GetTeamPieces(TEAM).GetCount() - 3 + std::min(blockedPawns, 9);

			result.Space[MIDGAME][TEAM] = count * weight * weight / 30;
			result.Space[ENDGAME][TEAM] = 0;
		}
		else
		{
			result.Space[MIDGAME][TEAM] = 0;
			result.Space[ENDGAME][TEAM] = 0;
		}
	}

	void EvaluateSpace(EvaluationResult& result, const Position& position)
	{
		BitBoard whitePawns = position.GetTeamPieces(TEAM_WHITE, PIECE_PAWN);
		BitBoard blackPawns = position.GetTeamPieces(TEAM_BLACK, PIECE_PAWN);
		BitBoard allPawns = whitePawns | blackPawns;

		int blockedPawns = 0;
		blockedPawns += (Shift<NORTH>(whitePawns) & (allPawns | GetPawnDoubleAttacks<TEAM_BLACK>(blackPawns)));
		blockedPawns += (Shift<SOUTH>(blackPawns) & (allPawns | GetPawnDoubleAttacks<TEAM_WHITE>(whitePawns)));

		EvaluateSpace<TEAM_WHITE>(result, position, blockedPawns);
		EvaluateSpace<TEAM_BLACK>(result, position, blockedPawns);
	}

	void EvaluateTempo(EvaluationResult& result, const Position& position)
	{
		if constexpr (UseInitiative)
		{
			constexpr ValueType tempo = 12;
			Team other = OtherTeam(position.TeamToPlay);
			result.Tempo[MIDGAME][position.TeamToPlay] = tempo;
			result.Tempo[ENDGAME][position.TeamToPlay] = tempo;
			result.Tempo[MIDGAME][other] = 0;
			result.Tempo[ENDGAME][other] = 0;
		}
		else
		{
			result.Tempo[MIDGAME][TEAM_WHITE] = 0;
			result.Tempo[ENDGAME][TEAM_WHITE] = 0;
			result.Tempo[MIDGAME][TEAM_BLACK] = 0;
			result.Tempo[ENDGAME][TEAM_BLACK] = 0;
		}
	}

	// =================================================================================================================================
	// ENTRY POINTS
	// =================================================================================================================================

	void ClearResult(EvaluationResult& result)
	{
		memset(&result, 0, sizeof(EvaluationResult));
	}

	EvaluationResult EvaluateDetailed(const Position& position, Team team, ValueType alpha, ValueType beta)
	{
		EvaluationResult result;
		result.GameStage = CalculateGameStage(position);
		result.IsDraw = false;
		EvaluateMaterial(result, position);
		EvaluatePieceSquareTables(result, position);
		
		BitBoard whiteDoublePawnAttacks = GetPawnDoubleAttacks<TEAM_WHITE>(position.GetTeamPieces(TEAM_WHITE, PIECE_PAWN));
		BitBoard blackDoublePawnAttacks = GetPawnDoubleAttacks<TEAM_BLACK>(position.GetTeamPieces(TEAM_BLACK, PIECE_PAWN));

		result.Data.AttackedBy[TEAM_WHITE][PIECE_PAWN] = GetPawnAttacks<TEAM_WHITE>(position.GetTeamPieces(TEAM_WHITE, PIECE_PAWN));
		result.Data.AttackedBy[TEAM_BLACK][PIECE_PAWN] = GetPawnAttacks<TEAM_BLACK>(position.GetTeamPieces(TEAM_BLACK, PIECE_PAWN));
		result.Data.AttackedBy[TEAM_WHITE][PIECE_KING] = GetNonSlidingAttacks<PIECE_KING>(position.GetKingSquare(TEAM_WHITE));
		result.Data.AttackedBy[TEAM_BLACK][PIECE_KING] = GetNonSlidingAttacks<PIECE_KING>(position.GetKingSquare(TEAM_BLACK));
		result.Data.AttackedByTwice[TEAM_WHITE] = whiteDoublePawnAttacks | (result.Data.AttackedBy[TEAM_WHITE][PIECE_PAWN] & result.Data.AttackedBy[TEAM_WHITE][PIECE_KING]);
		result.Data.AttackedByTwice[TEAM_BLACK] = blackDoublePawnAttacks | (result.Data.AttackedBy[TEAM_BLACK][PIECE_PAWN] & result.Data.AttackedBy[TEAM_BLACK][PIECE_KING]);

		result.Data.KingAttackZone[TEAM_WHITE] = CalculateKingAttackRegion<TEAM_WHITE>(position);
		result.Data.KingAttackZone[TEAM_BLACK] = CalculateKingAttackRegion<TEAM_BLACK>(position);

		result.Data.Attackers[TEAM_WHITE] = (result.Data.KingAttackZone[TEAM_BLACK] & result.Data.AttackedBy[TEAM_WHITE][PIECE_PAWN]).GetCount();
		result.Data.Attackers[TEAM_BLACK] = (result.Data.KingAttackZone[TEAM_WHITE] & result.Data.AttackedBy[TEAM_BLACK][PIECE_PAWN]).GetCount();

		result.Data.KingAttackZone[TEAM_WHITE] &= ~whiteDoublePawnAttacks;
		result.Data.KingAttackZone[TEAM_BLACK] &= ~blackDoublePawnAttacks;

		// Do these first as they set AttackedBy data
		EvaluatePieces<PIECE_KNIGHT>(result, position);
		EvaluatePieces<PIECE_BISHOP>(result, position);
		EvaluatePieces<PIECE_ROOK>(result, position);
		EvaluatePieces<PIECE_QUEEN>(result, position);

		EvaluatePawns(result, position);
		EvaluateBlockedPieces(result, position);
		EvaluateSpace(result, position);
		EvaluateKingSafety(result, position);
		EvaluateThreats(result, position);
		EvaluateTempo(result, position);

		return result;
	}

	ValueType Evaluate(const Position& position, Team team, ValueType alpha, ValueType beta)
	{
		return EvaluateDetailed(position, team, alpha, beta).GetTotal(team);
	}

	EvaluationResult EvaluateDetailed(const Position& position)
	{
		return EvaluateDetailed(position, TEAM_WHITE, -SCORE_MATE, SCORE_MATE);
	}

	ValueType Evaluate(const Position& position, Team team)
	{
		return Evaluate(position, team, -SCORE_MATE, SCORE_MATE);
	}

	bool IsEndgame(const Position& position)
	{
		return position.GetNonPawnMaterial() <= 1200;
	}

	ValueType GetPieceValue(const Position& position, Piece piece)
	{
		float gameStage = CalculateGameStage(position);
		ValueType mg = s_MaterialValues[MIDGAME][piece];
		ValueType eg = s_MaterialValues[ENDGAME][piece];
		return InterpolateGameStage(gameStage, mg, eg);
	}

	ValueType GetPieceValue(Piece piece, GameStage stage)
	{
		return s_MaterialValues[stage][piece];
	}

	// =================================================================================================================================
	//  FORMATTING
	// =================================================================================================================================

	std::string FormatScore(ValueType score, int maxPlaces)
	{
		int places = 1;
		if (std::abs(score) > 0)
			places = (int)log10(std::abs(score)) + 1;
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
		constexpr int SCORE_LENGTH = 6;

		ValueType totals[GAME_STAGE_MAX][TEAM_MAX];

		for (GameStage stage : { MIDGAME, ENDGAME })
		{
			for (Team team : { TEAM_WHITE, TEAM_BLACK })
			{
				totals[stage][team] =
					evaluation.Material[stage][team] +
					evaluation.PieceSquares[stage][team] +
					evaluation.BlockedPieces[stage][team] +
					evaluation.Pawns[stage][team] +
					evaluation.Knights[stage][team] +
					evaluation.Bishops[stage][team] +
					evaluation.Rooks[stage][team] +
					evaluation.Queens[stage][team] +
					evaluation.Space[stage][team] +
					evaluation.KingSafety[stage][team] +
					evaluation.Threats[stage][team] +
					evaluation.Tempo[stage][team];
			}
		}

#define FORMAT_TABLE_ROW(Score, Length) (FormatScore(Score[MIDGAME][TEAM_WHITE], Length) + " " + FormatScore(Score[ENDGAME][TEAM_WHITE], Length) + " | " + FormatScore(Score[MIDGAME][TEAM_BLACK], Length) + " " + FormatScore(Score[ENDGAME][TEAM_BLACK], Length) + " | " + FormatScore(Score[MIDGAME][TEAM_WHITE] - Score[MIDGAME][TEAM_BLACK], Length) + " " + FormatScore(Score[ENDGAME][TEAM_WHITE] - Score[ENDGAME][TEAM_BLACK], Length))
		std::string result = "";
		result += "       Term     |     White     |     Black     |     Total     \n";
		result += "                |   MG     EG   |   MG     EG   |   MG     EG   \n";
		result += " ---------------+---------------+---------------+---------------\n";
		result += "       Material | " + FORMAT_TABLE_ROW(evaluation.Material, SCORE_LENGTH)		+ '\n';
		result += "  Piece Squares | " + FORMAT_TABLE_ROW(evaluation.PieceSquares, SCORE_LENGTH)	+ '\n';
		result += " Blocked Pieces | " + FORMAT_TABLE_ROW(evaluation.BlockedPieces, SCORE_LENGTH)	+ '\n';
		result += "          Pawns | " + FORMAT_TABLE_ROW(evaluation.Pawns, SCORE_LENGTH)			+ '\n';
		result += "        Knights | " + FORMAT_TABLE_ROW(evaluation.Knights, SCORE_LENGTH)			+ '\n';
		result += "        Bishops | " + FORMAT_TABLE_ROW(evaluation.Bishops, SCORE_LENGTH)			+ '\n';
		result += "          Rooks | " + FORMAT_TABLE_ROW(evaluation.Rooks, SCORE_LENGTH)			+ '\n';
		result += "         Queens | " + FORMAT_TABLE_ROW(evaluation.Queens, SCORE_LENGTH)			+ '\n';
		result += "          Space | " + FORMAT_TABLE_ROW(evaluation.Space, SCORE_LENGTH)			+ '\n';
		result += "    King Safety | " + FORMAT_TABLE_ROW(evaluation.KingSafety, SCORE_LENGTH)		+ '\n';
		result += "        Threats | " + FORMAT_TABLE_ROW(evaluation.Threats, SCORE_LENGTH)			+ '\n';
		result += "          Tempo | " + FORMAT_TABLE_ROW(evaluation.Tempo, SCORE_LENGTH)			+ '\n';
		result += " ---------------+---------------+---------------+--------------\n";
		result += "          Total | " + FORMAT_TABLE_ROW(totals, SCORE_LENGTH)						+ "\n";
		result += "\n";
		result += "Game stage: " + std::to_string(evaluation.GameStage) + " / " + std::to_string(s_MaxPhaseValue) + '\n';
		result += "Total evaluation: " + std::to_string(evaluation.GetTotal(TEAM_WHITE)) + " (white side)";
		return result;
#undef FORMAT_TABLE_ROW
	}

}
