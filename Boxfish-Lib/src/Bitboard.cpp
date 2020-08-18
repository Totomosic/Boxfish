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

#ifdef BOX_PLATFORM_WINDOWS
	int BitBoard::GetCount() const
	{
		return (int)__popcnt64(Board);
	}
#else
	int BitBoard::GetCount() const
	{
		return (int)__builtin_popcountll(Board);
	}
#endif

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

	bool MoreThanOne(const BitBoard& board)
	{
		if (!board)
			return false;
		return ForwardBitScan(board) != BackwardBitScan(board);
	}

#ifdef BOX_PLATFORM_WINDOWS
	SquareIndex ForwardBitScan(const BitBoard& board)
	{
		unsigned long lsb;
		_BitScanForward64(&lsb, board.Board);
		return (SquareIndex)lsb;
	}

	SquareIndex BackwardBitScan(const BitBoard& board)
	{
		unsigned long msb;
		_BitScanReverse64(&msb, board.Board);
		return (SquareIndex)msb;
	}

	SquareIndex PopLeastSignificantBit(BitBoard& board)
	{
		unsigned long lsb;
		_BitScanForward64(&lsb, board.Board);
		board.Board &= board.Board - 1;
		return (SquareIndex)lsb;
	}

	BitBoard FlipVertically(const BitBoard& board)
	{
		return _byteswap_uint64(board.Board);
	}

#else

	SquareIndex ForwardBitScan(const BitBoard& board)
	{
		return (SquareIndex)(__builtin_ffsll(board.Board) - 1);
	}

	SquareIndex BackwardBitScan(const BitBoard& board)
	{
		return (SquareIndex)(63 - __builtin_clzll(board.Board));
	}

	SquareIndex PopLeastSignificantBit(BitBoard& board)
	{
		int lsbIndex = __builtin_ffsll(board.Board) - 1;
		board.Board &= board.Board - 1;
		return (SquareIndex)lsbIndex;
	}

	BitBoard FlipVertically(const BitBoard& board)
	{
		return __bswap_64(board.Board);
	}

#endif

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
