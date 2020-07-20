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
			if (move.GetFlags() & MOVE_CAPTURE)
			{
				score = 200 + GetPieceValue(move.GetCapturedPiece()) - GetPieceValue(move.GetMovingPiece());
			}
			else if (move.GetFlags() & MOVE_PROMOTION)
			{
				score = 150 + GetPieceValue(move.GetPromotionPiece());
			}
			else if (m_OrderingInfo.KillerMoves)
			{
				if (move == m_OrderingInfo.KillerMoves[0] || move == m_OrderingInfo.KillerMoves[1])
				{
					score = 100;
				}
			}

			// Counter move
			if (m_OrderingInfo.CounterMove != MOVE_NONE && move == m_OrderingInfo.CounterMove)
			{
				score += 50;
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

	void ScoreMovesQuiescence(const Position& position, MoveList& moves)
	{
		for (int i = 0; i < moves.MoveCount; i++)
		{
			Move& move = moves.Moves[i];
			if (move.GetFlags() & MOVE_CAPTURE)
			{
				move.SetValue(200 + GetPieceValue(move.GetCapturedPiece()) - GetPieceValue(move.GetMovingPiece()));
			}
			else if (move.GetFlags() & MOVE_PROMOTION)
			{
				move.SetValue(150 + GetPieceValue(move.GetPromotionPiece()));
			}
			else
			{
				move.SetValue(0);
			}
		}
	}

	QuiescenceMoveSelector::QuiescenceMoveSelector(const Position& position, MoveList& legalMoves)
		: m_LegalMoves(legalMoves), m_CurrentIndex(0), m_NumberOfCaptures(0), m_InCheck(position.InfoCache.CheckedBy[position.TeamToPlay])
	{
		ScoreMovesQuiescence(position, legalMoves);
		for (int i = 0; i < m_LegalMoves.MoveCount; i++)
		{
			const Move& m = m_LegalMoves.Moves[i];
			if (m_InCheck || (m.GetFlags() & MOVE_CAPTURE))
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
			Centipawns value = move.GetValue();
			if (value > bestScore && (m_InCheck || ((move.GetFlags() & MOVE_CAPTURE) || (move.GetFlags() & MOVE_PROMOTION && (move.GetPromotionPiece() == PIECE_QUEEN || move.GetPromotionPiece() == PIECE_KNIGHT)))))
			{
				bestScore = value;
				bestIndex = index;
			}
		}
		std::swap(m_LegalMoves.Moves[m_CurrentIndex], m_LegalMoves.Moves[bestIndex]);
		return m_LegalMoves.Moves[m_CurrentIndex++];
	}

}
