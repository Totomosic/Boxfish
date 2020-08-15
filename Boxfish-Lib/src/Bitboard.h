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

		int GetCount() const;
		// Methods only used when getting/setting from FEN
		bool GetAt(const Square& square) const;
		void SetAt(const Square& square);

		inline operator bool() const { return Board != 0ULL; }
		inline friend BitBoard operator&(const BitBoard& left, const BitBoard& right) { return left.Board & right.Board; }
		inline friend BitBoard operator|(const BitBoard& left, const BitBoard& right) { return left.Board | right.Board; }
		inline friend BitBoard operator^(const BitBoard& left, const BitBoard& right) { return left.Board ^ right.Board; }
		inline friend BitBoard operator&(const BitBoard& left, uint64_t right) { return left.Board & right; }
		inline friend BitBoard operator|(const BitBoard& left, uint64_t right) { return left.Board | right; }
		inline friend BitBoard operator^(const BitBoard& left, uint64_t right) { return left.Board ^ right; }
		inline friend BitBoard operator~(const BitBoard& board) { return ~board.Board; }
		inline friend BitBoard& operator&=(BitBoard& left, const BitBoard& right) { left.Board &= right.Board; return left; }
		inline friend BitBoard& operator|=(BitBoard& left, const BitBoard& right) { left.Board |= right.Board; return left; }
		inline friend BitBoard& operator^=(BitBoard& left, const BitBoard& right) { left.Board ^= right.Board; return left; }
		inline friend BitBoard operator<<(const BitBoard& left, int right) { return left.Board << right; }
		inline friend BitBoard operator>>(const BitBoard& left, int right) { return left.Board >> right; }
		friend std::ostream& operator<<(std::ostream& stream, const BitBoard& board);

	public:
		inline static SquareIndex SquareToBitIndex(const Square& square)
		{
			BOX_ASSERT(square.File >= 0 && square.File < FILE_MAX&& square.Rank >= 0 && square.Rank < RANK_MAX, "Invalid square");
			return (SquareIndex)(square.File + square.Rank * FILE_MAX);
		}

		inline static Square BitIndexToSquare(SquareIndex index) { return { FileOfIndex(index), RankOfIndex(index) }; }
		inline static Rank RankOfIndex(int index) { return (Rank)(index / FILE_MAX); }
		inline static File FileOfIndex(int index) { return (File)(index % FILE_MAX); }
	};

	constexpr BitBoard ZERO_BB = 0ULL;
	constexpr BitBoard ALL_SQUARES_BB = 0xFFFFFFFFFFFFFFFF;

	bool MoreThanOne(const BitBoard& board);
	SquareIndex ForwardBitScan(const BitBoard& board);
	SquareIndex BackwardBitScan(const BitBoard& board);
	SquareIndex PopLeastSignificantBit(BitBoard& board);
	BitBoard ShiftEast(const BitBoard& board, int count);
	BitBoard ShiftWest(const BitBoard& board, int count);
	BitBoard FlipVertically(const BitBoard& board);

	int GetForwardShift(Team team);

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

	constexpr BitBoard DARK_SQUARES_MASK = 0xAA55AA55AA55AA55;
	constexpr BitBoard LIGHT_SQUARES_MASK = 0x55AA55AA55AA55AA;

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

	inline BitBoard operator&(const BitBoard& left, SquareIndex right) { return left & SQUARE_BITBOARDS[right]; };
	inline BitBoard operator|(const BitBoard& left, SquareIndex right) { return left | SQUARE_BITBOARDS[right]; };
	inline BitBoard operator^(const BitBoard& left, SquareIndex right) { return left ^ SQUARE_BITBOARDS[right]; };
	inline BitBoard operator~(SquareIndex square) { return ~SQUARE_BITBOARDS[square]; }
	inline BitBoard& operator&=(BitBoard& left, SquareIndex right) { left.Board &= SQUARE_BITBOARDS[right].Board; return left; };
	inline BitBoard& operator|=(BitBoard& left, SquareIndex right) { left.Board |= SQUARE_BITBOARDS[right].Board; return left; };
	inline BitBoard& operator^=(BitBoard& left, SquareIndex right) { left.Board ^= SQUARE_BITBOARDS[right].Board; return left; };

	inline BitBoard operator<<(SquareIndex left, int right) { return SQUARE_BITBOARDS[left] << right; };
	inline BitBoard operator>>(SquareIndex left, int right) { return SQUARE_BITBOARDS[left] >> right; };

	inline BitBoard operator|(SquareIndex left, SquareIndex right) { return SQUARE_BITBOARDS[left] | SQUARE_BITBOARDS[right]; }

}
