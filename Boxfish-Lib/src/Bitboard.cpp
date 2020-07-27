#include "Bitboard.h"

namespace Boxfish
{

	bool BitBoard::GetAt(const Square& square) const
	{
		return (Board & BOX_BIT((uint64_t)SquareToBitIndex(square))) != 0;
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

	std::vector<Square> BitBoard::GetSquares() const
	{
		std::vector<Square> result;
		for (int i = 0; i < RANK_MAX * FILE_MAX; i++)
		{
			if (Board & BOX_BIT(i))
				result.push_back(BitIndexToSquare((SquareIndex)i));
		}
		return result;
	}

	void BitBoard::SetAt(const Square& square)
	{
		Board |= BOX_BIT(SquareToBitIndex(square));
	}

	void BitBoard::Reset()
	{
		Board = 0;
	}

	SquareIndex BitBoard::SquareToBitIndex(const Square& square)
	{
		BOX_ASSERT(square.File >= 0 && square.File < FILE_MAX && square.Rank >= 0 && square.Rank < RANK_MAX, "Invalid square");
		return (SquareIndex)(square.File + square.Rank * FILE_MAX);
	}

	Square BitBoard::BitIndexToSquare(SquareIndex index)
	{
		return { FileOfIndex(index), RankOfIndex(index) };
	}

	Rank BitBoard::RankOfIndex(int index)
	{
		return (Rank)(index / FILE_MAX);
	}

	File BitBoard::FileOfIndex(int index)
	{
		return (File)(index % FILE_MAX);
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
				stream << ((board.GetAt({ file, rank })) ? '1' : '0') << ' ';
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
		board &= board.Board - 1;
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
