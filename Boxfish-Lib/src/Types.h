#pragma once
#pragma warning( disable : 26812 )
#include "Logging.h"

namespace Boxfish
{

	enum Team : int8_t
	{
		TEAM_WHITE,
		TEAM_BLACK,
		TEAM_MAX
	};

	inline constexpr Team OtherTeam(Team current)
	{
		return (current == TEAM_WHITE) ? TEAM_BLACK : TEAM_WHITE;
	}

	enum File : int8_t
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

	enum Rank : int8_t
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

	constexpr Rank INVERSE_RANK[RANK_MAX] = {
		RANK_8,
		RANK_7,
		RANK_6,
		RANK_5,
		RANK_4,
		RANK_3,
		RANK_2,
		RANK_1
	};

	constexpr Rank RelativeRank(Team team, Rank whiteRank)
	{
		return (team == TEAM_WHITE) ? whiteRank : INVERSE_RANK[whiteRank];
	}

	inline File operator++(File& f, int)
	{
		f = (File)((int8_t)f + 1);
		return (File)((int8_t)f - 1);
	}

	inline Rank operator++(Rank& r, int)
	{
		r = (Rank)((int8_t)r + 1);
		return (Rank)((int8_t)r - 1);
	}

	inline File operator--(File& f, int)
	{
		f = (File)((int8_t)f - 1);
		return (File)((int8_t)f + 1);
	}

	inline Rank operator--(Rank& r, int)
	{
		r = (Rank)((int8_t)r - 1);
		return (Rank)((int8_t)r + 1);
	}

	enum Piece : int8_t
	{
		PIECE_PAWN,
		PIECE_KNIGHT, // Knight must be second for game stage calculation
		PIECE_BISHOP,
		PIECE_ROOK,
		PIECE_QUEEN,
		PIECE_KING, // King must be last piece for material eval
		PIECE_MAX,
		PIECE_ALL,
		PIECE_INVALID
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
		p = (Piece)((int8_t)p + 1);
		return (Piece)((int8_t)p - 1);
	}

	inline Piece operator--(Piece& p, int)
	{
		p = (Piece)((int8_t)p - 1);
		return (Piece)((int8_t)p + 1);
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

	enum GameStage : int8_t
	{
		MIDGAME,
		ENDGAME,
		GAME_STAGE_MAX
	};

	constexpr Square INVALID_SQUARE = { FILE_INVALID, RANK_INVALID };

	enum SquareIndex : int 
	{
		a1, b1, c1, d1, e1, f1, g1, h1,
		a2, b2, c2, d2, e2, f2, g2, h2,
		a3, b3, c3, d3, e3, f3, g3, h3,
		a4, b4, c4, d4, e4, f4, g4, h4,
		a5, b5, c5, d5, e5, f5, g5, h5,
		a6, b6, c6, d6, e6, f6, g6, h6,
		a7, b7, c7, d7, e7, f7, g7, h7,
		a8, b8, c8, d8, e8, f8, g8, h8,
		SQUARE_MAX
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
