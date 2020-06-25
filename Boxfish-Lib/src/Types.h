#pragma once
#include "BoxfishDefines.h"
#include "Logging.h"
#include <cstdint>

namespace Boxfish
{
	
	enum File : int
	{
		FILE_A,
		FILE_B,
		FILE_C,
		FILE_D,
		FILE_E,
		FILE_F,
		FILE_G,
		FILE_H,
		FILE_MAX,
		FILE_INVALID = -1
	};

	enum Rank : int
	{
		RANK_1,
		RANK_2,
		RANK_3,
		RANK_4,
		RANK_5,
		RANK_6,
		RANK_7,
		RANK_8,
		RANK_MAX,
		RANK_INVALID = -1
	};

	inline File operator++(File& f, int)
	{
		f = (File)((int)f + 1);
		return (File)((int)f - 1);
	}

	inline Rank operator++(Rank& r, int)
	{
		r = (Rank)((int)r + 1);
		return (Rank)((int)r - 1);
	}

	inline File operator--(File& f, int)
	{
		f = (File)((int)f - 1);
		return (File)((int)f + 1);
	}

	inline Rank operator--(Rank& r, int)
	{
		r = (Rank)((int)r - 1);
		return (Rank)((int)r + 1);
	}

	enum Piece : int
	{
		PIECE_PAWN,
		PIECE_KNIGHT,
		PIECE_BISHOP,
		PIECE_ROOK,
		PIECE_QUEEN,
		PIECE_KING,
		PIECE_MAX
	};

	inline bool IsSlidingPiece(Piece piece)
	{
		switch (piece)
		{
		case PIECE_PAWN:
			return false;
		case PIECE_KNIGHT:
			return false;
		case PIECE_BISHOP:
			return true;
		case PIECE_ROOK:
			return true;
		case PIECE_QUEEN:
			return true;
		case PIECE_KING:
			return true;
		default:
			BOX_ASSERT(false, "Invalid piece");
		}
		return false;
	}

	inline Piece operator++(Piece& p, int)
	{
		p = (Piece)((int)p + 1);
		return (Piece)((int)p - 1);
	}

	inline Piece operator--(Piece& p, int)
	{
		p = (Piece)((int)p - 1);
		return (Piece)((int)p + 1);
	}

	enum Team : int
	{
		TEAM_WHITE,
		TEAM_BLACK,
		TEAM_MAX
	};

	inline Team OtherTeam(Team current)
	{
		return (current == TEAM_WHITE) ? TEAM_BLACK : TEAM_WHITE;
	}

	struct BOX_API Square
	{
	public:
		::Boxfish::File File;
		::Boxfish::Rank Rank;
	};

	inline bool operator==(const Square& left, const Square& right)
	{
		return left.File == right.File && left.Rank == right.Rank;
	}

	inline bool operator!=(const Square& left, const Square& right)
	{
		return !(left == right);
	}

	enum GameStage
	{
		MIDGAME,
		ENDGAME,
		GAME_STAGE_MAX
	};

	constexpr Square INVALID_SQUARE = { FILE_INVALID, RANK_INVALID };

	constexpr uint64_t RANK_1_MASK = 0xffull;
	constexpr uint64_t RANK_2_MASK = 0xff00ull;
	constexpr uint64_t RANK_3_MASK = 0xff0000ull;
	constexpr uint64_t RANK_4_MASK = 0xff000000ull;
	constexpr uint64_t RANK_5_MASK = 0xff00000000ull;
	constexpr uint64_t RANK_6_MASK = 0xff0000000000ull;
	constexpr uint64_t RANK_7_MASK = 0xff000000000000ull;
	constexpr uint64_t RANK_8_MASK = 0xff00000000000000ull;
	constexpr uint64_t FILE_H_MASK = 0x8080808080808080ull;
	constexpr uint64_t FILE_G_MASK = 0x4040404040404040ull;
	constexpr uint64_t FILE_F_MASK = 0x2020202020202020ull;
	constexpr uint64_t FILE_E_MASK = 0x1010101010101010ull;
	constexpr uint64_t FILE_D_MASK = 0x808080808080808ull;
	constexpr uint64_t FILE_C_MASK = 0x404040404040404ull;
	constexpr uint64_t FILE_B_MASK = 0x202020202020202ull;
	constexpr uint64_t FILE_A_MASK = 0x101010101010101ull;

	constexpr uint64_t DARK_SQUARES_MASK = 0xAA55AA55AA55AA55;
	constexpr uint64_t LIGHT_SQUARES_MASK = 0x55AA55AA55AA55AA;

	enum SquareIndex : int 
	{
		a1, b1, c1, d1, e1, f1, g1, h1,
		a2, b2, c2, d2, e2, f2, g2, h2,
		a3, b3, c3, d3, e3, f3, g3, h3,
		a4, b4, c4, d4, e4, f4, g4, h4,
		a5, b5, c5, d5, e5, f5, g5, h5,
		a6, b6, c6, d6, e6, f6, g6, h6,
		a7, b7, c7, d7, e7, f7, g7, h7,
		a8, b8, c8, d8, e8, f8, g8, h8
	};

	inline SquareIndex operator++(SquareIndex& index, int)
	{
		index = (SquareIndex)(index + 1);
		return (SquareIndex)(index - 1);
	}

	inline SquareIndex operator--(SquareIndex& index, int)
	{
		index = (SquareIndex)(index - 1);
		return (SquareIndex)(index + 1);
	}

}
