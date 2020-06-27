#include "ZobristHash.h"
#include "Position.h"
#include <random>

namespace Boxfish
{

	static bool s_Initialized = false;

	static constexpr uint32_t s_Seed = 0xDABDABDA;

	uint64_t s_PieceOnSquare[TEAM_MAX][PIECE_MAX][FILE_MAX * RANK_MAX];
	uint64_t s_BlackToMove;
	uint64_t s_CastlingRights[4];
	uint64_t s_EnPassantFile[FILE_MAX];

	void InitZobristHash()
	{
		if (!s_Initialized)
		{
			std::mt19937 mt;
			mt.seed(s_Seed);
			std::uniform_int_distribution<uint64_t> dist;

			for (Piece piece = PIECE_PAWN; piece < PIECE_MAX; piece++)
			{
				for (SquareIndex index = a1; index < FILE_MAX * RANK_MAX; index++)
				{
					s_PieceOnSquare[TEAM_WHITE][piece][index] = dist(mt);
					s_PieceOnSquare[TEAM_BLACK][piece][index] = dist(mt);
				}
			}
			s_BlackToMove = dist(mt);
			s_CastlingRights[0] = dist(mt);
			s_CastlingRights[1] = dist(mt);
			s_CastlingRights[2] = dist(mt);
			s_CastlingRights[3] = dist(mt);
			for (File file = FILE_A; file < FILE_MAX; file++)
			{
				s_EnPassantFile[file] = dist(mt);
			}
			s_Initialized = true;
		}
	}

	ZobristHash operator^(const ZobristHash& left, const ZobristHash& right)
	{
		return left.Hash ^ right.Hash;
	}

	bool operator==(const ZobristHash& left, const ZobristHash& right)
	{
		return left.Hash == right.Hash;
	}

	bool operator!=(const ZobristHash& left, const ZobristHash& right)
	{
		return left.Hash != right.Hash;
	}

	ZobristHash::ZobristHash()
		: Hash(0ULL)
	{
	}

	ZobristHash::ZobristHash(uint64_t hash)
		: Hash(hash)
	{
	}

	void ZobristHash::SetFromPosition(const Position& pos)
	{
		Position position = pos;
		Hash = 0ULL;
		for (Piece piece = PIECE_PAWN; piece < PIECE_MAX; piece++)
		{
			BitBoard& whitePieces = position.Teams[TEAM_WHITE].Pieces[piece];
			while (whitePieces)
			{
				SquareIndex index = PopLeastSignificantBit(whitePieces);
				AddPieceAt(TEAM_WHITE, piece, index);
			}
			BitBoard& blackPieces = position.Teams[TEAM_BLACK].Pieces[piece];
			while (blackPieces)
			{
				SquareIndex index = PopLeastSignificantBit(blackPieces);
				AddPieceAt(TEAM_BLACK, piece, index);
			}
		}
		if (position.EnpassantSquare != INVALID_SQUARE)
		{
			AddEnPassant(position.EnpassantSquare.File);
		}
		if (position.TeamToPlay == TEAM_BLACK)
		{
			FlipTeamToPlay();
		}
		if (position.Teams[TEAM_WHITE].CastleKingSide)
			AddCastleKingside(TEAM_WHITE);
		if (position.Teams[TEAM_WHITE].CastleQueenSide)
			AddCastleQueenside(TEAM_WHITE);
		if (position.Teams[TEAM_BLACK].CastleKingSide)
			AddCastleKingside(TEAM_BLACK);
		if (position.Teams[TEAM_BLACK].CastleQueenSide)
			AddCastleQueenside(TEAM_BLACK);
	}

	void ZobristHash::RemovePieceAt(Team team, Piece piece, SquareIndex square)
	{
		Hash ^= s_PieceOnSquare[team][piece][square];
	}

	void ZobristHash::AddPieceAt(Team team, Piece piece, SquareIndex square)
	{
		Hash ^= s_PieceOnSquare[team][piece][square];
	}

	void ZobristHash::FlipTeamToPlay()
	{
		Hash ^= s_BlackToMove;
	}

	void ZobristHash::RemoveEnPassant(File file)
	{
		Hash ^= s_EnPassantFile[file];
	}

	void ZobristHash::AddEnPassant(File file)
	{
		Hash ^= s_EnPassantFile[file];
	}

	void ZobristHash::AddCastleKingside(Team team)
	{
		if (team == TEAM_WHITE)
			Hash ^= s_CastlingRights[0];
		else
			Hash ^= s_CastlingRights[2];
	}

	void ZobristHash::AddCastleQueenside(Team team)
	{
		if (team == TEAM_WHITE)
			Hash ^= s_CastlingRights[1];
		else
			Hash ^= s_CastlingRights[3];
	}

	void ZobristHash::RemoveCastleKingside(Team team)
	{
		AddCastleKingside(team);
	}

	void ZobristHash::RemoveCastleQueenside(Team team)
	{
		AddCastleQueenside(team);
	}

	ZobristHash& ZobristHash::operator^=(const ZobristHash& right)
	{
		Hash ^= right.Hash;
		return *this;
	}

}
