#include "MoveSelector.h"
#include "Evaluation.h"

namespace Boxfish
{

	constexpr Centipawns SCORE_GOOD_CAPTURE = 100000000;
	constexpr Centipawns SCORE_PROMOTION = 90000000;
	constexpr Centipawns SCORE_KILLER = 80000000;
	constexpr Centipawns SCORE_BAD_CAPTURE = 50000000;

	MoveSelector::MoveSelector(const MoveOrderingInfo& info, MoveList* legalMoves)
		: m_OrderingInfo(info), m_LegalMoves(legalMoves), m_CurrentIndex(0)
	{
		for (int index = m_CurrentIndex; index < m_LegalMoves->MoveCount; index++)
		{
			Centipawns score = 0;
			Move& move = m_LegalMoves->Moves[index];
			if (move.IsCapture())
			{
				if (SeeGE(*m_OrderingInfo.CurrentPosition, move))
				{
					score = SCORE_GOOD_CAPTURE + GetPieceValue(move.GetCapturedPiece()) - GetPieceValue(move.GetMovingPiece());
				}
				else
				{
					score = SCORE_BAD_CAPTURE + GetPieceValue(move.GetCapturedPiece()) - GetPieceValue(move.GetMovingPiece());
				}
			}
			else if (move.IsPromotion())
			{
				score = SCORE_PROMOTION + GetPieceValue(move.GetPromotionPiece());
			}
			else if (m_OrderingInfo.KillerMoves)
			{
				if (move == m_OrderingInfo.KillerMoves[1])
					score = SCORE_KILLER - 1;
				else if (move == m_OrderingInfo.KillerMoves[0])
					score = SCORE_KILLER;
			}

			// Counter move
			if (move == m_OrderingInfo.CounterMove)
			{
				score += 20;
			}
			// History Heuristic
			if (m_OrderingInfo.HistoryTable && m_OrderingInfo.ButterflyTable)
			{
				Centipawns history = (*m_OrderingInfo.HistoryTable)[m_OrderingInfo.CurrentPosition->TeamToPlay][move.GetFromSquareIndex()][move.GetToSquareIndex()];
				Centipawns butterfly = (*m_OrderingInfo.ButterflyTable)[m_OrderingInfo.CurrentPosition->TeamToPlay][move.GetFromSquareIndex()][move.GetToSquareIndex()];
				if (history != 0 && butterfly != 0)
				{
					score += history / butterfly;
				}
			}
			move.SetValue(score);
		}
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
			const Move& move = m_LegalMoves->Moves[index];
			if (move.GetValue() > bestScore)
			{
				bestIndex = index;
				bestScore = move.GetValue();
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

	void ScoreMovesQuiescence(const Position& position, MoveList& moves)
	{
		for (int i = 0; i < moves.MoveCount; i++)
		{
			Move& move = moves.Moves[i];
			if (move.GetFlags() & MOVE_CAPTURE)
			{
				move.SetValue(SCORE_GOOD_CAPTURE + GetPieceValue(move.GetCapturedPiece()) - GetPieceValue(move.GetMovingPiece()));
			}
			else if (move.GetFlags() & MOVE_PROMOTION)
			{
				move.SetValue(SCORE_PROMOTION + GetPieceValue(move.GetPromotionPiece()));
			}
			else
			{
				move.SetValue(0);
			}
		}
	}

	QuiescenceMoveSelector::QuiescenceMoveSelector(const Position& position, MoveList& legalMoves)
		: m_LegalMoves(legalMoves), m_CurrentIndex(0), m_NumberOfCaptures(0), m_InCheck(IsInCheck(position, position.TeamToPlay))
	{
		ScoreMovesQuiescence(position, legalMoves);
		if (m_InCheck)
		{
			m_NumberOfCaptures = m_LegalMoves.MoveCount;
		}
		else
		{
			for (int i = 0; i < m_LegalMoves.MoveCount; i++)
			{
				const Move& m = m_LegalMoves.Moves[i];
				if (m.GetFlags() & MOVE_CAPTURE)
					m_NumberOfCaptures++;
			}
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
			Centipawns value = move.GetValue();
			if (value > bestScore && (m_InCheck || ((move.IsCapture()) || ((move.IsPromotion()) && (move.GetPromotionPiece() == PIECE_QUEEN || move.GetPromotionPiece() == PIECE_KNIGHT)))))
			{
				bestScore = value;
				bestIndex = index;
			}
		}
		std::swap(m_LegalMoves.Moves[m_CurrentIndex], m_LegalMoves.Moves[bestIndex]);
		return m_LegalMoves.Moves[m_CurrentIndex++];
	}

}
