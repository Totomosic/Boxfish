#pragma once
#include "BoxfishDefines.h"
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

	struct BOX_API Square
	{
	public:
		Boxfish::File File;
		Boxfish::Rank Rank;
	};

	inline bool operator==(const Square& left, const Square& right)
	{
		return left.File == right.File && left.Rank == right.Rank;
	}

	inline bool operator!=(const Square& left, const Square& right)
	{
		return !(left == right);
	}

	constexpr Square INVALID_SQUARE = { FILE_INVALID, RANK_INVALID };

}
