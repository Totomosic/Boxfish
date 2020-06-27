#include "MoveSelector.h"
#include "Evaluation.h"

namespace Boxfish
{

	MoveSelector::MoveSelector(MoveList& legalMoves)
		: m_LegalMoves(legalMoves), m_CurrentIndex(0)
	{
	}

	bool MoveSelector::Empty() const
	{
		return m_CurrentIndex >= m_LegalMoves.MoveCount;
	}

	Move MoveSelector::GetNextMove()
	{
		BOX_ASSERT(!Empty(), "Move list is empty");
		return m_LegalMoves.Moves[m_CurrentIndex++];
	}

	void ScoreMoves(const Position& position, MoveList& moves, int* moveScores)
	{
		for (int i = 0; i < moves.MoveCount; i++)
		{
			Move& move = moves.Moves[i];
			if (move.GetFlags() & MOVE_CAPTURE)
			{
				moveScores[i] = 20 + GetPieceValue(position, move.GetCapturedPiece()) - GetPieceValue(position, move.GetMovingPiece());
			}
			else if (move.GetFlags() & MOVE_PROMOTION)
			{
				moveScores[i] = 50 + GetPieceValue(position, move.GetPromotionPiece());
			}
		}
	}

	QuiescenceMoveSelector::QuiescenceMoveSelector(const Position& position, MoveList& legalMoves)
		: m_LegalMoves(legalMoves), m_MoveScores(), m_CurrentIndex(0), m_NumberOfCaptures(0)
	{
		ScoreMoves(position, legalMoves, m_MoveScores);
		for (int i = 0; i < m_LegalMoves.MoveCount; i++)
		{
			Move& m = m_LegalMoves.Moves[i];
			if (m.GetFlags() & MOVE_CAPTURE)
				m_NumberOfCaptures++;
		}
	}

	bool QuiescenceMoveSelector::Empty() const
	{
		return m_CurrentIndex >= m_NumberOfCaptures;
	}

	Move QuiescenceMoveSelector::GetNextMove()
	{
		BOX_ASSERT(!Empty(), "Move list is empty");
		size_t bestIndex = 0;
		Centipawns bestScore = -INF;
		for (int index = m_CurrentIndex; index < m_LegalMoves.MoveCount; index++)
		{
			const Move& move = m_LegalMoves.Moves[index];
			Centipawns value = m_MoveScores[index];
			if (value > bestScore && (move.GetFlags() & (MOVE_CAPTURE | MOVE_PROMOTION)))
			{
				bestScore = value;
				bestIndex = index;
			}
		}
		std::swap(m_LegalMoves.Moves[m_CurrentIndex], m_LegalMoves.Moves[bestIndex]);
		return m_LegalMoves.Moves[m_CurrentIndex++];
	}

}
