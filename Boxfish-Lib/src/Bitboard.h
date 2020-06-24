#pragma once
#include "Logging.h"
#include "Types.h"
#include <vector>
#include <bit>

namespace Boxfish
{

	class BOX_API BitBoard
	{
	public:
		uint64_t Board;

	public:
		BitBoard();
		BitBoard(uint64_t board);
		BitBoard(SquareIndex index);

		bool GetAt(const Square& square) const;
		int GetCount() const;
		std::vector<Square> GetSquares() const;
		void SetAt(const Square& square);
		void Flip(const Square& square);
		void Invert();

		void Reset();

		operator bool() const;
		friend BitBoard operator&(const BitBoard& left, const BitBoard& right);
		friend BitBoard operator|(const BitBoard& left, const BitBoard& right);
		friend BitBoard operator^(const BitBoard& left, const BitBoard& right);
		friend BitBoard operator&(const BitBoard& left, uint64_t right);
		friend BitBoard operator|(const BitBoard& left, uint64_t right);
		friend BitBoard operator^(const BitBoard& left, uint64_t right);
		friend BitBoard operator~(const BitBoard& board);
		friend BitBoard& operator&=(BitBoard& left, const BitBoard& right);
		friend BitBoard& operator|=(BitBoard& left, const BitBoard& right);
		friend BitBoard& operator^=(BitBoard& left, const BitBoard& right);
		friend BitBoard operator<<(const BitBoard& left, int right);
		friend BitBoard operator>>(const BitBoard& left, int right);
		friend std::ostream& operator<<(std::ostream& stream, const BitBoard& board);

	public:
		static SquareIndex SquareToBitIndex(const Square& square);
		static Square BitIndexToSquare(SquareIndex index);
		static Rank RankOfIndex(int index);
		static File FileOfIndex(int index);
	};

	SquareIndex ForwardBitScan(const BitBoard& board);
	SquareIndex BackwardBitScan(const BitBoard& board);
	SquareIndex PopLeastSignificantBit(BitBoard& board);
	BitBoard ShiftEast(const BitBoard& board, int count);
	BitBoard ShiftWest(const BitBoard& board, int count);

	int GetForwardShift(Team team);

}
