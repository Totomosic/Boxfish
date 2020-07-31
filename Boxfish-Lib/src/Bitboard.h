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

		constexpr BitBoard(SquareIndex index)
			: Board(1ULL << (int)index)
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

}
