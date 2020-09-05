#include "MoveSelector.h"
#include "Evaluation.h"

namespace Boxfish
{

	constexpr Centipawns SCORE_GOOD_CAPTURE = 500;
	constexpr Centipawns SCORE_PROMOTION = 600;
	constexpr Centipawns SCORE_KILLER = 300;
	constexpr Centipawns SCORE_BAD_CAPTURE = -1000;

	MoveSelector::MoveSelector(MoveList* moves, const Position* currentPosition, const Move& ttMove, const Move& counterMove, const OrderingTables* tables, const Move* killers)
		: m_Moves(moves), m_CurrentPosition(currentPosition), m_ttMove(ttMove), m_CounterMove(counterMove), m_Tables(tables), m_Killers(killers), m_CurrentIndex(0), m_CurrentStage(TTMove)
	{
		if (ttMove == MOVE_NONE)
			m_CurrentStage++;
		for (int index = m_CurrentIndex; index < m_Moves->MoveCount; index++)
		{
			Centipawns score = 0;
			Move& move = m_Moves->Moves[index];
			if (move.IsCapture())
			{
				if (SeeGE(*currentPosition, move, -30))
				{
					score = SCORE_GOOD_CAPTURE + GetPieceValue(move.GetCapturedPiece());
				}
				else
				{
					score = SCORE_BAD_CAPTURE - GetPieceValue(move.GetMovingPiece());
				}
			}
			else if (move.IsPromotion())
			{
				score = SCORE_PROMOTION + GetPieceValue(move.GetPromotionPiece());
			}
			else if (killers)
			{
				if (move == killers[1])
					score = SCORE_KILLER - 1;
				else if (move == killers[0])
					score = SCORE_KILLER;
			}

			// Counter move
			if (move == counterMove)
			{
				score += 25;
			}
			// History Heuristic
			if (m_Tables->History && m_Tables->Butterfly)
			{
				Centipawns history = m_Tables->History[currentPosition->TeamToPlay][move.GetFromSquareIndex()][move.GetToSquareIndex()];
				Centipawns butterfly = m_Tables->Butterfly[currentPosition->TeamToPlay][move.GetFromSquareIndex()][move.GetToSquareIndex()];
				if (butterfly != 0 && history > butterfly)
				{
					score += history / butterfly;
				}
			}
			move.SetValue(score);
		}
	}

	Move MoveSelector::GetNextMove()
	{
		if (m_CurrentStage == TTMove && m_ttMove != MOVE_NONE)
		{
			m_CurrentStage++;
			return m_ttMove;
		}
		if (m_CurrentIndex >= m_Moves->MoveCount)
			return MOVE_NONE;
		int bestIndex = m_CurrentIndex;
		Centipawns bestScore = -SCORE_MATE;
		for (int index = m_CurrentIndex; index < m_Moves->MoveCount; index++)
		{
			const Move& move = m_Moves->Moves[index];
			if (move != m_ttMove && move.GetValue() > bestScore)
			{
				bestIndex = index;
				bestScore = move.GetValue();
				if (bestScore == SCORE_MATE)
					break;
			}
		}
		if (m_CurrentIndex != bestIndex)
		{
			std::swap(m_Moves->Moves[m_CurrentIndex], m_Moves->Moves[bestIndex]);
		}
		Move mv = m_Moves->Moves[m_CurrentIndex++];
		if (mv == m_ttMove)
			return GetNextMove();
		return mv;
	}

	void ScoreMovesQuiescence(const Position& position, MoveList& moves)
	{
		for (int i = 0; i < moves.MoveCount; i++)
		{
			Move& move = moves.Moves[i];
			if (move.IsCapture())
			{
				if (SeeGE(position, move, -30))
					move.SetValue(SCORE_GOOD_CAPTURE + GetPieceValue(move.GetCapturedPiece()));
				else
					move.SetValue(SCORE_NONE);
			}
			else if (move.IsPromotion() && (move.GetPromotionPiece() == PIECE_QUEEN || move.GetPromotionPiece() == PIECE_KNIGHT))
			{
				move.SetValue(SCORE_PROMOTION + GetPieceValue(move.GetPromotionPiece()));
			}
			else
			{
				move.SetValue(SCORE_NONE);
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
				if (m.IsCaptureOrPromotion() && m.GetValue() > SCORE_NONE)
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
		Centipawns bestScore = SCORE_NONE;
		for (int index = m_CurrentIndex; index < m_LegalMoves.MoveCount; index++)
		{
			const Move& move = m_LegalMoves.Moves[index];
			Centipawns value = move.GetValue();
			if (value > bestScore)
			{
				bestScore = value;
				bestIndex = index;
			}
		}
		std::swap(m_LegalMoves.Moves[m_CurrentIndex], m_LegalMoves.Moves[bestIndex]);
		return m_LegalMoves.Moves[m_CurrentIndex++];
	}

}
