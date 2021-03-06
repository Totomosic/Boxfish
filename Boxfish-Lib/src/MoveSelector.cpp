#include "MoveSelector.h"
#include "Evaluation.h"

namespace Boxfish
{

	constexpr ValueType SCORE_GOOD_CAPTURE = 10000;
	constexpr ValueType SCORE_PROMOTION = 3000;
	constexpr ValueType SCORE_BAD_PROMOTION = -500;
	constexpr ValueType SCORE_KILLER = 2000;
	constexpr ValueType SCORE_BAD_CAPTURE = -1000;

	MoveSelector::MoveSelector(MoveList* moves, const Position* currentPosition, Move ttMove, Move counterMove, Move prevMove, const OrderingTables* tables, const Move* killers)
		: m_Moves(moves), m_CurrentPosition(currentPosition), m_ttMove(ttMove), m_CounterMove(counterMove), m_Tables(tables), m_Killers(killers), m_CurrentIndex(0), m_CurrentStage(TTMove)
	{
		if (ttMove == MOVE_NONE)
			m_CurrentStage++;
		for (int index = m_CurrentIndex; index < m_Moves->MoveCount; ++index)
		{
			ValueType score = 0;
			Move& move = m_Moves->Moves[index];
			if (move.IsCapture())
			{
				if (SeeGE(*currentPosition, move, -30))
				{
					score = SCORE_GOOD_CAPTURE + GetPieceValue(move.GetCapturedPiece()) - GetPieceValue(move.GetMovingPiece());
				}
				else
				{
					score = SCORE_BAD_CAPTURE + GetPieceValue(move.GetCapturedPiece()) - GetPieceValue(move.GetMovingPiece());
				}
				if (prevMove != MOVE_NONE && move.GetToSquareIndex() == prevMove.GetToSquareIndex())
					score += 250; // Recapture
			}
			else if (move.IsPromotion())
			{
				Piece promotion = move.GetPromotionPiece();
				if (promotion == PIECE_QUEEN || promotion == PIECE_KNIGHT)
					score = SCORE_PROMOTION + GetPieceValue(promotion);
				else
					score = SCORE_BAD_PROMOTION + GetPieceValue(promotion);
			}
			else
			{
				if (killers)
				{
					if (move == killers[0])
						score = SCORE_KILLER;
					else if (move == killers[1])
						score = SCORE_KILLER - 200;
				}

				if (move.IsAdvancedPawnPush(currentPosition->TeamToPlay) || (move.GetFlags() & (MOVE_DOUBLE_PAWN_PUSH | MOVE_KINGSIDE_CASTLE | MOVE_QUEENSIDE_CASTLE)))
				{
					score += 250;
					if (move.GetMovingPiece() == PIECE_PAWN && BitBoard::FileOfIndex(move.GetToSquareIndex()) == FILE_H)
						score += 45;
				}

				// Central pawn push
				constexpr BitBoard Center = (RANK_4_MASK | RANK_5_MASK) & (FILE_C_MASK | FILE_D_MASK | FILE_E_MASK | FILE_F_MASK);
				if (move.GetMovingPiece() == PIECE_PAWN && (Center & move.GetToSquareIndex()))
					score += 50;

				// Counter move
				if (move == counterMove)
				{
					score += 1500;
				}
				// History Heuristic
				ValueType history = m_Tables->History[currentPosition->TeamToPlay][move.GetFromSquareIndex()][move.GetToSquareIndex()];
				ValueType butterfly = m_Tables->Butterfly[currentPosition->TeamToPlay][move.GetFromSquareIndex()][move.GetToSquareIndex()];
				if (butterfly != 0 && history > butterfly)
				{
					score += history * 50 / butterfly;
				}
			}

			move.SetValue(score);
		}
	}

	Move MoveSelector::GetNextMove(bool skipQuiets)
	{
		if (m_CurrentStage == TTMove && m_ttMove != MOVE_NONE)
		{
			m_CurrentStage++;
			return m_ttMove;
		}
		if (m_CurrentIndex >= m_Moves->MoveCount)
			return MOVE_NONE;
		int bestIndex = m_CurrentIndex;
		ValueType bestScore = -SCORE_MATE;
		ValueType value;
		for (int index = m_CurrentIndex; index < m_Moves->MoveCount; ++index)
		{
			const Move& move = m_Moves->Moves[index];
			value = move.GetValue();
			if (value > bestScore)
			{
				bestIndex = index;
				bestScore = value;
			}
		}
		if (m_CurrentIndex != bestIndex)
		{
			std::swap(m_Moves->Moves[m_CurrentIndex], m_Moves->Moves[bestIndex]);
		}
		Move mv = m_Moves->Moves[m_CurrentIndex++];
		if (mv == m_ttMove)
			return GetNextMove(skipQuiets);
		return mv;
	}

	void ScoreMoveQuiescence(const Position& position, Move& move)
	{
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

	void ScoreMovesQuiescence(const Position& position, MoveList& moves)
	{
		for (int i = 0; i < moves.MoveCount; ++i)
		{
			Move& move = moves.Moves[i];
			ScoreMoveQuiescence(position, move);
		}
	}

	QuiescenceMoveSelector::QuiescenceMoveSelector(const Position& position, MoveList& legalMoves, bool generateChecks)
		: m_LegalMoves(legalMoves), m_CurrentIndex(0), m_NumberOfCaptures(0), m_InCheck(generateChecks)
	{
		ScoreMovesQuiescence(position, m_LegalMoves);
		if (m_InCheck)
		{
			m_NumberOfCaptures = m_LegalMoves.MoveCount;
		}
		else
		{
			for (int i = 0; i < m_LegalMoves.MoveCount; ++i)
			{
				const Move& m = m_LegalMoves.Moves[i];
				if (m.GetValue() > SCORE_NONE)
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
		ValueType bestScore = SCORE_NONE - 1;
		ValueType value;
		for (int index = m_CurrentIndex; index < m_LegalMoves.MoveCount; ++index)
		{
			const Move& move = m_LegalMoves.Moves[index];
			value = move.GetValue();
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
