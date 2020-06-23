#include "Move.h"

namespace Boxfish
{

	Move::Move()
		: m_Move(0)
	{
	}

	Move::Move(const MoveDefinition& definition)
		: m_Move(0)
	{
		m_Move = ((definition.Flags & 0x7f) << 21) | ((BitBoard::SquareToBitIndex(definition.ToSquare) & 0x3f) << 15) | ((BitBoard::SquareToBitIndex(definition.FromSquare) & 0x3f) << 9) | (definition.MovingPiece & 0x7);
	}

	MoveDefinition Move::GetDefinition() const
	{
		MoveDefinition definition;
		definition.FromSquare = GetFromSquare();
		definition.ToSquare = GetToSquare();
		definition.MovingPiece = GetMovingPiece();
		definition.Flags = GetFlags();
		return definition;
	}

	Square Move::GetFromSquare() const
	{
		return BitBoard::BitIndexToSquare((m_Move >> 9) & 0x3f);
	}

	Square Move::GetToSquare() const
	{
		return BitBoard::BitIndexToSquare((m_Move >> 15) & 0x3f);
	}

	Piece Move::GetMovingPiece() const
	{
		return (Piece)(m_Move & 0x7);
	}

	MoveFlag Move::GetFlags() const
	{
		return (MoveFlag)((m_Move >> 21) & 0x7f);
	}

	void Move::SetFlags(MoveFlag flags)
	{
		m_Move = m_Move | (flags << 21);
	}

	Piece Move::GetCapturedPiece() const
	{
		return (Piece)((m_Move >> 6) & 0x7);
	}

	void Move::SetCapturedPiece(Piece piece)
	{
		uint32_t mask = 0x7 << 6;
		m_Move = (m_Move & ~mask) | ((piece << 6) & mask);
	}

	Piece Move::GetPromotionPiece() const
	{
		return (Piece)((m_Move >> 3) & 0x7);
	}

	void Move::SetPromotionPiece(Piece piece)
	{
		uint32_t mask = 0x7 << 3;
		m_Move = (m_Move & ~mask) | ((piece << 3) & mask);
	}

}
