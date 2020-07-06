#include "MoveSelector.h"
#include "Evaluation.h"

namespace Boxfish
{

	std::string FormatLine(const Line& line, bool includeSymbols)
	{
		std::string result = "";
		for (size_t i = 0; i < line.CurrentIndex; i++)
		{
			result += FormatMove(line.Moves[i], includeSymbols) + " ";
		}
		return result;
	}

	MoveSelector::MoveSelector(const MoveOrderingInfo& info, MoveList* legalMoves)
		: m_OrderingInfo(info), m_LegalMoves(legalMoves), m_CurrentIndex(0)
	{
	}

	bool MoveSelector::Empty() const
	{
		return m_CurrentIndex >= m_LegalMoves->MoveCount;
	}

	Move MoveSelector::GetNextMove()
	{
		BOX_ASSERT(!Empty(), "Move list is empty");
		int bestIndex = m_CurrentIndex;
		Centipawns bestScore = -SCORE_MATE;
		for (int index = m_CurrentIndex; index < m_LegalMoves->MoveCount; index++)
		{
			Centipawns score = 0;
			const Move& move = m_LegalMoves->Moves[index];
			if (m_OrderingInfo.PVMove == move)
			{
				score = SCORE_MATE;
			}
			else if (move.GetFlags() & MOVE_CAPTURE)
			{
				score = 20 + GetPieceValue(move.GetCapturedPiece()) - GetPieceValue(move.GetMovingPiece());
			}
			else if (move.GetFlags() & MOVE_PROMOTION)
			{
				score = 50 + GetPieceValue(move.GetPromotionPiece());
			}
			else if (m_OrderingInfo.MoveEvaluator && m_OrderingInfo.CurrentPosition)
			{
				score = m_OrderingInfo.MoveEvaluator(*m_OrderingInfo.CurrentPosition, move);
			}
			if (score > bestScore)
			{
				bestIndex = index;
				bestScore = score;
				if (bestScore == SCORE_MATE)
					break;
			}
		}
		if (m_CurrentIndex != bestIndex)
		{
			std::swap(m_LegalMoves->Moves[m_CurrentIndex], m_LegalMoves->Moves[bestIndex]);
		}
		return m_LegalMoves->Moves[m_CurrentIndex++];
	}

	void ScoreMovesQuiescence(const Position& position, MoveList& moves, int* moveScores)
	{
		for (int i = 0; i < moves.MoveCount; i++)
		{
			Move& move = moves.Moves[i];
			if (move.GetFlags() & MOVE_CAPTURE)
			{
				moveScores[i] = 20 + GetPieceValue(move.GetCapturedPiece()) - GetPieceValue(move.GetMovingPiece());
			}
			else if (move.GetFlags() & MOVE_PROMOTION)
			{
				moveScores[i] = 50 + GetPieceValue(move.GetPromotionPiece());
			}
		}
	}

	QuiescenceMoveSelector::QuiescenceMoveSelector(const Position& position, MoveList& legalMoves)
		: m_LegalMoves(legalMoves), m_MoveScores(), m_CurrentIndex(0), m_NumberOfCaptures(0)
	{
		ScoreMovesQuiescence(position, legalMoves, m_MoveScores);
		for (int i = 0; i < m_LegalMoves.MoveCount; i++)
		{
			const Move& m = m_LegalMoves.Moves[i];
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
		Centipawns bestScore = -SCORE_MATE;
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
