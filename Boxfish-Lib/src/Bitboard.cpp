#include "Bitboard.h"

namespace Boxfish
{

	bool BitBoard::GetAt(const Square& square) const
	{
		return (*this) & BitBoard::SquareToBitIndex(square);
	}

	void BitBoard::SetAt(const Square& square)
	{
		(*this) |= BitBoard::SquareToBitIndex(square);
	}

	std::ostream& operator<<(std::ostream& stream, const BitBoard& board)
	{
		stream << "   | A B C D E F G H" << std::endl;
		stream << "---------------------" << std::endl;
		for (Rank rank = RANK_8; rank >= RANK_1; rank--)
		{
			stream << ' ' << (char)('1' + (rank - RANK_1)) << ' ' << '|' << ' ';
			for (File file = FILE_A; file < FILE_MAX; file++)
			{
				stream << ((board & BitBoard::SquareToBitIndex({ file, rank })) ? '1' : '0') << ' ';
			}
			if (rank != RANK_1)
				stream << std::endl;
		}
		return stream;
	}

	BitBoard ShiftEast(const BitBoard& board, int count)
	{
		BitBoard newBoard = board;
		for (int i = 0; i < count; i++)
		{
			newBoard = ((newBoard << 1) & (~FILE_A_MASK));
		}
		return newBoard;
	}

	BitBoard ShiftWest(const BitBoard& board, int count)
	{
		BitBoard newBoard = board;
		for (int i = 0; i < count; i++)
		{
			newBoard = ((newBoard >> 1) & (~FILE_H_MASK));
		}
		return newBoard;
	}

	int GetForwardShift(Team team)
	{
		return (team == TEAM_WHITE) ? FILE_MAX : -FILE_MAX;
	}

}
