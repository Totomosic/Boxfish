#include "Bitboard.h"

namespace Boxfish
{

	BitBoard::BitBoard()
		: Board(0)
	{
	}

	BitBoard::BitBoard(uint64_t board)
		: Board(board)
	{
	}

	bool BitBoard::GetAt(const Square& square) const
	{
		return (Board & BOX_BIT((uint64_t)SquareToBitIndex(square))) != 0;
	}

	int BitBoard::GetCount() const
	{
		// TODO: Use popcount
		int count = 0;
		for (int i = 0; i < RANK_MAX * FILE_MAX; i++)
		{
			if (Board & BOX_BIT(i))
				count++;
		}
		return count;
	}

	std::vector<Square> BitBoard::GetSquares() const
	{
		std::vector<Square> result;
		for (int i = 0; i < RANK_MAX * FILE_MAX; i++)
		{
			if (Board & BOX_BIT(i))
				result.push_back(BitIndexToSquare(i));
		}
		return result;
	}

	void BitBoard::SetAt(const Square& square)
	{
		Board |= BOX_BIT(SquareToBitIndex(square));
	}

	void BitBoard::Flip(const Square& square)
	{
		Board ^= BOX_BIT(SquareToBitIndex(square));
	}

	void BitBoard::Invert()
	{
		Board = ~Board;
	}

	void BitBoard::Reset()
	{
		Board = 0;
	}

	int BitBoard::SquareToBitIndex(const Square& square) const
	{
		BOX_ASSERT(square.File >= 0 && square.File < FILE_MAX && square.Rank >= 0 && square.Rank < RANK_MAX, "Invalid square");
		return square.File + square.Rank * FILE_MAX;
	}

	Square BitBoard::BitIndexToSquare(int index) const
	{
		return { (File)(index % FILE_MAX), (Rank)(index / FILE_MAX) };
	}

}
