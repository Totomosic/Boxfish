#include "Bitboard.h"

namespace Boxfish
{

	bool BitBoard::GetAt(const Square& square) const
	{
		return (Board & BOX_BIT((uint64_t)SquareToBitIndex(square))) != 0;
	}

	int BitBoard::GetCount() const
	{
		// TODO: Use popcount
		return (int)__popcnt64(Board);
		/*int count = 0;
		for (int i = 0; i < RANK_MAX * FILE_MAX; i++)
		{
			if (Board & BOX_BIT(i))
				count++;
		}
		return count;*/
	}

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

	BitBoard::operator bool() const
	{
		return Board != 0;
	}

	BitBoard operator&(const BitBoard& left, const BitBoard& right)
	{
		return left.Board & right.Board;
	}

	BitBoard operator|(const BitBoard& left, const BitBoard& right)
	{
		return left.Board | right.Board;
	}

	BitBoard operator^(const BitBoard& left, const BitBoard& right)
	{
		return left.Board ^ right.Board;
	}

	BitBoard operator&(const BitBoard& left, uint64_t right)
	{
		return left.Board & right;
	}

	BitBoard operator|(const BitBoard& left, uint64_t right)
	{
		return left.Board | right;
	}

	BitBoard operator^(const BitBoard& left, uint64_t right)
	{
		return left.Board ^ right;
	}

	BitBoard operator~(const BitBoard& board)
	{
		return ~board.Board;
	}

	BitBoard& operator&=(BitBoard& left, const BitBoard& right)
	{
		left.Board &= right.Board;
		return left;
	}

	BitBoard& operator|=(BitBoard& left, const BitBoard& right)
	{
		left.Board |= right.Board;
		return left;
	}

	BitBoard& operator^=(BitBoard& left, const BitBoard& right)
	{
		left.Board ^= right.Board;
		return left;
	}

	BitBoard operator<<(const BitBoard& left, int right)
	{
		return BitBoard{ left.Board << right };
	}

	BitBoard operator>>(const BitBoard& left, int right)
	{
		return BitBoard{ left.Board >> right };
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

	SquareIndex ForwardBitScan(const BitBoard& board)
	{
		unsigned long lsb;
		unsigned char success = _BitScanForward64(&lsb, board.Board);
		if (success == 0)
			return (SquareIndex)-1;
		return (SquareIndex)lsb;
	}

	SquareIndex BackwardBitScan(const BitBoard& board)
	{
		unsigned long msb;
		unsigned char success = _BitScanReverse64(&msb, board.Board);
		if (success == 0)
			return (SquareIndex)-1;
		return (SquareIndex)msb;
	}

	SquareIndex PopLeastSignificantBit(BitBoard& board)
	{
		unsigned long lsb;
		unsigned char success = _BitScanForward64(&lsb, board.Board);
		if (success == 0)
			return (SquareIndex)-1;
		board &= board.Board - 1;
		return (SquareIndex)lsb;
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
