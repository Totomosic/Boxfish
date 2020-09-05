#pragma once
#include "Logging.h"
#include "Types.h"
#include <vector>

namespace Boxfish
{

	class BOX_API BitBoard
	{
	public:
		uint64_t Board;

	public:
		constexpr BitBoard()
			: Board(0)
		{
		}

		constexpr BitBoard(uint64_t board)
			: Board(board)
		{
		}

#ifdef BOX_PLATFORM_WINDOWS
		inline int GetCount() const
		{
			return (int)__popcnt64(Board);
		}
#else
		inline int GetCount() const
		{
			return (int)__builtin_popcountll(Board);
		}
#endif
		// Methods only used when getting/setting from FEN
		bool GetAt(const Square& square) const;
		void SetAt(const Square& square);

		inline constexpr operator bool() const { return Board != 0ULL; }
		inline constexpr friend BitBoard operator&(const BitBoard& left, const BitBoard& right) { return left.Board & right.Board; }
		inline constexpr friend BitBoard operator|(const BitBoard& left, const BitBoard& right) { return left.Board | right.Board; }
		inline constexpr friend BitBoard operator^(const BitBoard& left, const BitBoard& right) { return left.Board ^ right.Board; }
		inline constexpr friend BitBoard operator&(const BitBoard& left, uint64_t right) { return left.Board & right; }
		inline constexpr friend BitBoard operator|(const BitBoard& left, uint64_t right) { return left.Board | right; }
		inline constexpr friend BitBoard operator^(const BitBoard& left, uint64_t right) { return left.Board ^ right; }
		inline constexpr friend BitBoard operator~(const BitBoard& board) { return ~board.Board; }
		inline constexpr friend BitBoard& operator&=(BitBoard& left, const BitBoard& right) { left.Board &= right.Board; return left; }
		inline constexpr friend BitBoard& operator|=(BitBoard& left, const BitBoard& right) { left.Board |= right.Board; return left; }
		inline constexpr friend BitBoard& operator^=(BitBoard& left, const BitBoard& right) { left.Board ^= right.Board; return left; }
		inline constexpr friend BitBoard operator<<(const BitBoard& left, int right) { return left.Board << right; }
		inline constexpr friend BitBoard operator>>(const BitBoard& left, int right) { return left.Board >> right; }
		friend std::ostream& operator<<(std::ostream& stream, const BitBoard& board);

	public:
		constexpr static SquareIndex SquareToBitIndex(const Square& square)
		{
			BOX_ASSERT(square.File >= 0 && square.File < FILE_MAX&& square.Rank >= 0 && square.Rank < RANK_MAX, "Invalid square");
			return (SquareIndex)(square.File + square.Rank * FILE_MAX);
		}

		constexpr static Square BitIndexToSquare(SquareIndex index) { return { FileOfIndex(index), RankOfIndex(index) }; }
		constexpr static Rank RankOfIndex(int index) { return (Rank)(index >> 3); }
		constexpr static File FileOfIndex(int index) { return (File)(index & 7); }
	};

#ifdef BOX_PLATFORM_WINDOWS
	inline SquareIndex ForwardBitScan(const BitBoard& board)
	{
		unsigned long lsb;
		_BitScanForward64(&lsb, board.Board);
		return (SquareIndex)lsb;
	}

	inline SquareIndex BackwardBitScan(const BitBoard& board)
	{
		unsigned long msb;
		_BitScanReverse64(&msb, board.Board);
		return (SquareIndex)msb;
	}

	inline BitBoard FlipVertically(const BitBoard& board)
	{
		return _byteswap_uint64(board.Board);
	}

#else

	inline SquareIndex ForwardBitScan(const BitBoard& board)
	{
		return (SquareIndex)(__builtin_ffsll(board.Board) - 1);
	}

	inline SquareIndex BackwardBitScan(const BitBoard& board)
	{
		return (SquareIndex)(63 - __builtin_clzll(board.Board));
	}

	inline BitBoard FlipVertically(const BitBoard& board)
	{
		return __bswap_64(board.Board);
	}

#endif

	inline SquareIndex PopLeastSignificantBit(BitBoard& board)
	{
		const SquareIndex sq = ForwardBitScan(board);
		board.Board &= board.Board - 1;
		return sq;
	}

	constexpr bool MoreThanOne(const BitBoard& board)
	{
		return board.Board & (board.Board - 1);
	}

	BitBoard ShiftEast(const BitBoard& board, int count);
	BitBoard ShiftWest(const BitBoard& board, int count);

	int GetForwardShift(Team team);

	inline SquareIndex FrontmostSquare(const BitBoard& board, Team team) { return (team == TEAM_WHITE) ? BackwardBitScan(board) : ForwardBitScan(board); }
	inline SquareIndex BackmostSquare(const BitBoard& board, Team team) { return (team == TEAM_WHITE) ? ForwardBitScan(board) : BackwardBitScan(board); }

	constexpr BitBoard ZERO_BB = 0ULL;
	constexpr BitBoard ALL_SQUARES_BB = 0xFFFFFFFFFFFFFFFFULL;

	constexpr BitBoard RANK_1_MASK = 0xffull;
	constexpr BitBoard RANK_2_MASK = 0xff00ull;
	constexpr BitBoard RANK_3_MASK = 0xff0000ull;
	constexpr BitBoard RANK_4_MASK = 0xff000000ull;
	constexpr BitBoard RANK_5_MASK = 0xff00000000ull;
	constexpr BitBoard RANK_6_MASK = 0xff0000000000ull;
	constexpr BitBoard RANK_7_MASK = 0xff000000000000ull;
	constexpr BitBoard RANK_8_MASK = 0xff00000000000000ull;
	constexpr BitBoard FILE_H_MASK = 0x8080808080808080ull;
	constexpr BitBoard FILE_G_MASK = 0x4040404040404040ull;
	constexpr BitBoard FILE_F_MASK = 0x2020202020202020ull;
	constexpr BitBoard FILE_E_MASK = 0x1010101010101010ull;
	constexpr BitBoard FILE_D_MASK = 0x808080808080808ull;
	constexpr BitBoard FILE_C_MASK = 0x404040404040404ull;
	constexpr BitBoard FILE_B_MASK = 0x202020202020202ull;
	constexpr BitBoard FILE_A_MASK = 0x101010101010101ull;

	constexpr BitBoard DARK_SQUARES_MASK = 0xAA55AA55AA55AA55;
	constexpr BitBoard LIGHT_SQUARES_MASK = 0x55AA55AA55AA55AA;

	constexpr BitBoard FILE_MASKS[FILE_MAX] = {
		FILE_A_MASK,
		FILE_B_MASK,
		FILE_C_MASK,
		FILE_D_MASK,
		FILE_E_MASK,
		FILE_F_MASK,
		FILE_G_MASK,
		FILE_H_MASK,
	};

	constexpr BitBoard RANK_MASKS[RANK_MAX] = {
		RANK_1_MASK,
		RANK_2_MASK,
		RANK_3_MASK,
		RANK_4_MASK,
		RANK_5_MASK,
		RANK_6_MASK,
		RANK_7_MASK,
		RANK_8_MASK,
	};

	// From white perspective
	constexpr BitBoard INFRONT_BB[RANK_MAX] = {
		RANK_1_MASK | RANK_2_MASK | RANK_3_MASK | RANK_4_MASK | RANK_5_MASK | RANK_6_MASK | RANK_7_MASK | RANK_8_MASK,
		RANK_2_MASK | RANK_3_MASK | RANK_4_MASK | RANK_5_MASK | RANK_6_MASK | RANK_7_MASK | RANK_8_MASK,
		RANK_3_MASK | RANK_4_MASK | RANK_5_MASK | RANK_6_MASK | RANK_7_MASK | RANK_8_MASK,
		RANK_4_MASK | RANK_5_MASK | RANK_6_MASK | RANK_7_MASK | RANK_8_MASK,
		RANK_5_MASK | RANK_6_MASK | RANK_7_MASK | RANK_8_MASK,
		RANK_6_MASK | RANK_7_MASK | RANK_8_MASK,
		RANK_7_MASK | RANK_8_MASK,
		RANK_8_MASK,
	};

	// From white perspective
	constexpr BitBoard BEHIND_BB[RANK_MAX] = {
		RANK_1_MASK,
		RANK_1_MASK | RANK_2_MASK,
		RANK_1_MASK | RANK_2_MASK | RANK_3_MASK,
		RANK_1_MASK | RANK_2_MASK | RANK_3_MASK | RANK_4_MASK,
		RANK_1_MASK | RANK_2_MASK | RANK_3_MASK | RANK_4_MASK | RANK_5_MASK,
		RANK_1_MASK | RANK_2_MASK | RANK_3_MASK | RANK_4_MASK | RANK_5_MASK | RANK_6_MASK,
		RANK_1_MASK | RANK_2_MASK | RANK_3_MASK | RANK_4_MASK | RANK_5_MASK | RANK_6_MASK | RANK_7_MASK,
		RANK_1_MASK | RANK_2_MASK | RANK_3_MASK | RANK_4_MASK | RANK_5_MASK | RANK_6_MASK | RANK_7_MASK | RANK_8_MASK,
	};

	// Includes rank
	constexpr BitBoard InFront(Rank rank, Team team)
	{
		return (team == TEAM_WHITE) ? INFRONT_BB[rank] : BEHIND_BB[rank];
	}

	// Includes rank
	constexpr BitBoard Behind(Rank rank, Team team)
	{
		return (team == TEAM_WHITE) ? BEHIND_BB[rank] : INFRONT_BB[rank];
	}

	enum Direction
	{
		NORTH,
		SOUTH,
		EAST,
		WEST,
		NORTH_EAST,
		NORTH_WEST,
		SOUTH_EAST,
		SOUTH_WEST
	};

	template<Direction D>
	constexpr BitBoard Shift(const BitBoard& b)
	{
		return  D == NORTH ? b << 8 : D == SOUTH ? b >> 8
			: D == EAST ? (b & ~FILE_H_MASK) << 1 : D == WEST ? (b & ~FILE_A_MASK) >> 1
			: D == NORTH_EAST ? (b & ~FILE_H_MASK) << 9 : D == NORTH_WEST ? (b & ~FILE_A_MASK) << 7
			: D == SOUTH_EAST ? (b & ~FILE_H_MASK) >> 7 : D == SOUTH_WEST ? (b & ~FILE_A_MASK) >> 9
			: ZERO_BB;
	}

	constexpr BitBoard SQUARE_BITBOARDS[FILE_MAX * RANK_MAX] = {
		1ULL << a1, 1ULL << b1, 1ULL << c1, 1ULL << d1, 1ULL << e1, 1ULL << f1, 1ULL << g1, 1ULL << h1,
		1ULL << a2, 1ULL << b2, 1ULL << c2, 1ULL << d2, 1ULL << e2, 1ULL << f2, 1ULL << g2, 1ULL << h2,
		1ULL << a3, 1ULL << b3, 1ULL << c3, 1ULL << d3, 1ULL << e3, 1ULL << f3, 1ULL << g3, 1ULL << h3,
		1ULL << a4, 1ULL << b4, 1ULL << c4, 1ULL << d4, 1ULL << e4, 1ULL << f4, 1ULL << g4, 1ULL << h4,
		1ULL << a5, 1ULL << b5, 1ULL << c5, 1ULL << d5, 1ULL << e5, 1ULL << f5, 1ULL << g5, 1ULL << h5,
		1ULL << a6, 1ULL << b6, 1ULL << c6, 1ULL << d6, 1ULL << e6, 1ULL << f6, 1ULL << g6, 1ULL << h6,
		1ULL << a7, 1ULL << b7, 1ULL << c7, 1ULL << d7, 1ULL << e7, 1ULL << f7, 1ULL << g7, 1ULL << h7,
		1ULL << a8, 1ULL << b8, 1ULL << c8, 1ULL << d8, 1ULL << e8, 1ULL << f8, 1ULL << g8, 1ULL << h8,
	};

	inline constexpr BitBoard operator&(const BitBoard& left, SquareIndex right) { return left & SQUARE_BITBOARDS[right]; };
	inline constexpr BitBoard operator|(const BitBoard& left, SquareIndex right) { return left | SQUARE_BITBOARDS[right]; };
	inline constexpr BitBoard operator^(const BitBoard& left, SquareIndex right) { return left ^ SQUARE_BITBOARDS[right]; };
	inline constexpr BitBoard operator~(SquareIndex square) { return ~SQUARE_BITBOARDS[square]; }
	inline constexpr BitBoard& operator&=(BitBoard& left, SquareIndex right) { left.Board &= SQUARE_BITBOARDS[right].Board; return left; };
	inline constexpr BitBoard& operator|=(BitBoard& left, SquareIndex right) { left.Board |= SQUARE_BITBOARDS[right].Board; return left; };
	inline constexpr BitBoard& operator^=(BitBoard& left, SquareIndex right) { left.Board ^= SQUARE_BITBOARDS[right].Board; return left; };

	inline constexpr BitBoard operator<<(SquareIndex left, int right) { return SQUARE_BITBOARDS[left] << right; };
	inline constexpr BitBoard operator>>(SquareIndex left, int right) { return SQUARE_BITBOARDS[left] >> right; };

	inline constexpr BitBoard operator|(SquareIndex left, SquareIndex right) { return SQUARE_BITBOARDS[left] | SQUARE_BITBOARDS[right]; }

}
