#include "Move.h"
#include "PositionUtils.h"
#include "Utils.h"

namespace Boxfish
{

	Square Move::GetFromSquare() const
	{
		return BitBoard::BitIndexToSquare(GetFromSquareIndex());
	}

	Square Move::GetToSquare() const
	{
		return BitBoard::BitIndexToSquare(GetToSquareIndex());
	}

	void Move::SetFlags(MoveFlag flags)
	{
		m_Move = m_Move | (flags << 21);
	}

	void Move::SetCapturedPiece(Piece piece)
	{
		constexpr uint32_t mask = 0x7 << 6;
		m_Move = (m_Move & ~mask) | ((piece << 6) & mask);
	}

	void Move::SetPromotionPiece(Piece piece)
	{
		constexpr uint32_t mask = 0x7 << 3;
		m_Move = (m_Move & ~mask) | ((piece << 3) & mask);
	}

}
