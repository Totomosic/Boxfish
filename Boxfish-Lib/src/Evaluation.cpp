#include "Evaluation.h"
#include "MoveGenerator.h"
#include "Attacks.h"

namespace Boxfish
{

	static bool s_Initialized = false;

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
			s_PawnShieldMasks[TEAM_WHITE][sq] = ((square >> 8) | ((square >> 7) & ~FILE_H_MASK) | ((square >> 9) & ~FILE_A_MASK)) & RANK_7_MASK;
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

	void InitEvaluation()
	{
		if (!s_Initialized)
		{
			InitPhase();
			InitDistanceTable();
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

	inline Centipawns InterpolateGameStage(float stage, Centipawns midgame, Centipawns endgame)
	{
		return (Centipawns)(midgame + (endgame - midgame) * stage);
	}

	template<Team team>
	void EvaluateMaterial(EvaluationResult& result, const Position& position, float stage)
	{
		Centipawns mg = 0;
		Centipawns eg = 0;
		for (Piece piece = PIECE_PAWN; piece < PIECE_MAX; piece++)
		{
			Centipawns midgame = s_MaterialValues[MIDGAME][piece];
			Centipawns endgame = s_MaterialValues[ENDGAME][piece];
			int count = position.GetTeamPieces(team, piece).GetCount();
			mg += midgame * count;
			eg += endgame * count;
		}
		result.Material[team] = InterpolateGameStage(stage, mg, eg);
	}

	void EvaluateMaterial(EvaluationResult& result, const Position& position, float stage)
	{
		EvaluateMaterial<TEAM_WHITE>(result, position, stage);
		EvaluateMaterial<TEAM_BLACK>(result, position, stage);
	}

	template<Team team>
	void EvaluatePieceSquareTables(EvaluationResult& result, const Position& position, float stage)
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
		EvaluatePieceSquareTables<TEAM_WHITE>(result, position, stage);
		EvaluatePieceSquareTables<TEAM_BLACK>(result, position, stage);
	}

	template<Team team>
	void EvaluateAttacks(EvaluationResult& result, const Position& position, float stage)
	{
		BitBoard attacks = ZERO_BB;

		BitBoard blockers = position.GetAllPieces();
		BitBoard bishops = position.GetTeamPieces(team, PIECE_BISHOP);
		while (bishops)
		{
			SquareIndex square = PopLeastSignificantBit(bishops);
			attacks |= GetSlidingAttacks(PIECE_BISHOP, square, blockers);
		}

		int count = attacks.GetCount();
		Centipawns mg = count * 2;
		Centipawns eg = count * 3;
		result.Attacks[team] = InterpolateGameStage(stage, mg, eg);
	}

	void EvaluateAttacks(EvaluationResult& result, const Position& position, float stage)
	{
		EvaluateAttacks<TEAM_WHITE>(result, position, stage);
		EvaluateAttacks<TEAM_BLACK>(result, position, stage);
	}

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
	void EvaluatePawnShield(EvaluationResult& result, const Position& position, float stage)
	{
		SquareIndex kingSquare = position.InfoCache.KingSquare[team];
		int count = (s_PawnShieldMasks[team][kingSquare] & position.GetTeamPieces(team, PIECE_PAWN)).GetCount();

		File kingFile = BitBoard::BitIndexToSquare(kingSquare).File;
		if ((position.GetTeamPieces(team, PIECE_PAWN) & s_NeighbourFiles[kingFile]) == ZERO_BB)
		{
			count -= 2;
		}

		constexpr Centipawns mg = 20;
		constexpr Centipawns eg = 0;
		result.PawnShield[team] = InterpolateGameStage(stage, mg * count, eg * count);
	}

	void EvaluatePawnShield(EvaluationResult& result, const Position& position, float stage)
	{
		EvaluatePawnShield<TEAM_WHITE>(result, position, stage);
		EvaluatePawnShield<TEAM_BLACK>(result, position, stage);
	}

	template<Team team>
	void EvaluatePassedPawns(EvaluationResult& result, const Position& position, float stage)
	{
		Centipawns value = 0;
		BitBoard pawns = position.GetTeamPieces(team, PIECE_PAWN);
		constexpr Team otherTeam = OtherTeam(team);

		BitBoard otherPawns = position.GetTeamPieces(otherTeam, PIECE_PAWN);

		while (pawns)
		{
			SquareIndex square = PopLeastSignificantBit(pawns);
			if ((otherPawns & s_PassedPawnMasks[team][square]) == ZERO_BB)
			{
				Rank rank = BitBoard::BitIndexToSquare(square).Rank;
				Centipawns mg = s_PassedPawnRankWeights[MIDGAME][team][rank];
				Centipawns eg = s_PassedPawnRankWeights[ENDGAME][team][rank];
				value += InterpolateGameStage(stage, mg, eg);
			}
		}
		result.PassedPawns[team] = value;
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
		Centipawns mg = -20 * count;
		Centipawns eg = -30 * count;
		result.DoubledPawns[team] = InterpolateGameStage(stage, mg, eg);
	}

	void EvaluateDoubledPawns(EvaluationResult& result, const Position& position, float stage)
	{
		EvaluateDoubledPawns<TEAM_WHITE>(result, position, stage);
		EvaluateDoubledPawns<TEAM_BLACK>(result, position, stage);
	}

	template<Team team>
	void EvaluateRooksOnOpenFiles(EvaluationResult& result, const Position& position, float stage)
	{
		int count = 0;
		BitBoard rooks = position.GetTeamPieces(team, PIECE_ROOK);
		BitBoard allPawns = position.GetPieces(PIECE_PAWN);
		while (rooks)
		{
			SquareIndex square = PopLeastSignificantBit(rooks);
			File file = BitBoard::BitIndexToSquare(square).File;
			if ((allPawns & FILE_MASKS[file]) == ZERO_BB)
			{
				count++;
			}
		}
		Centipawns mg = 10 * count;
		Centipawns eg = 50 * count;
		result.RooksOnOpenFiles[team] = InterpolateGameStage(stage, mg, eg);
	}

	void EvaluateRooksOnOpenFiles(EvaluationResult& result, const Position& position, float stage)
	{
		EvaluateRooksOnOpenFiles<TEAM_WHITE>(result, position, stage);
		EvaluateRooksOnOpenFiles<TEAM_BLACK>(result, position, stage);
	}

	template<Team team>
	void EvaluateKingSafety(EvaluationResult& result, const Position& position, float stage)
	{
		// Pawn storm
		Team otherTeam = OtherTeam(team);
		SquareIndex kingSquare = position.InfoCache.KingSquare[team];
		Square kingSq = BitBoard::BitIndexToSquare(kingSquare);
		BitBoard fileMasks = FILE_MASKS[kingSq.File] | s_NeighbourFiles[kingSq.File];
		int multiplier = (team == TEAM_WHITE) ? 1 : -1;

		BitBoard ranks = RANK_MASKS[kingSq.Rank];
		for (int i = 1; i < 4; i++)
		{
			File rank = (File)(kingSq.Rank + i * multiplier);
			if (rank < 0 || rank >= RANK_MAX)
			{
				break;
			}
			ranks |= RANK_MASKS[rank];
		}
		int count = (position.GetTeamPieces(otherTeam, PIECE_PAWN) & (fileMasks & ranks)).GetCount();
		constexpr Centipawns mg = -10;
		constexpr Centipawns eg = 0;
		result.KingSafety[team] = InterpolateGameStage(stage, mg * count, eg * count);
	}

	void EvaluateKingSafety(EvaluationResult& result, const Position& position, float stage)
	{
		EvaluateKingSafety<TEAM_WHITE>(result, position, stage);
		EvaluateKingSafety<TEAM_BLACK>(result, position, stage);
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
			EvaluateAttacks(result, position, gameStage);
			EvaluatePieceSquareTables(result, position, gameStage);
			EvaluatePawnShield(result, position, gameStage);
			EvaluatePassedPawns(result, position, gameStage);
			EvaluateDoubledPawns(result, position, gameStage);
			EvaluateRooksOnOpenFiles(result, position, gameStage);
			EvaluateKingSafety(result, position, gameStage);
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
