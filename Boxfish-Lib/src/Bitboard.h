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

		bool GetAt(const Square& square) const;
		int GetCount() const;
		std::vector<Square> GetSquares() const;
		void SetAt(const Square& square);

		void Reset();

		inline operator bool() const { return Board != 0; }
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
		static SquareIndex SquareToBitIndex(const Square& square);
		static Square BitIndexToSquare(SquareIndex index);
		static Rank RankOfIndex(int index);
		static File FileOfIndex(int index);
	};

	constexpr BitBoard ALL_SQUARES_BB = 0xFFFFFFFFFFFFFFFF;

	bool MoreThanOne(const BitBoard& board);
	SquareIndex ForwardBitScan(const BitBoard& board);
	SquareIndex BackwardBitScan(const BitBoard& board);
	SquareIndex PopLeastSignificantBit(BitBoard& board);
	BitBoard ShiftEast(const BitBoard& board, int count);
	BitBoard ShiftWest(const BitBoard& board, int count);

	int GetForwardShift(Team team);

}
