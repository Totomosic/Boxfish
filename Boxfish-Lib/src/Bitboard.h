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

		bool GetAt(const Square& square) const;
		int GetCount() const;
		std::vector<Square> GetSquares() const;
		void SetAt(const Square& square);
		void Flip(const Square& square);
		void Invert();

		void Reset();
	private:
		int SquareToBitIndex(const Square& square) const;
		Square BitIndexToSquare(int index) const;
	};

}
