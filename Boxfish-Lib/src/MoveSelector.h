#pragma once
#include "Move.h"
#include "MoveGenerator.h"
#include "Evaluation.h"

namespace Boxfish
{

	struct BOX_API MoveOrderingInfo
	{
	public:
		const Position* CurrentPosition = nullptr;
		const Move* KillerMoves = nullptr;
		Move PreviousMove = MOVE_NONE;
		Move CounterMove = MOVE_NONE;
		Centipawns (*HistoryTable)[TEAM_MAX][FILE_MAX * RANK_MAX][FILE_MAX * RANK_MAX];
		Centipawns (*ButterflyTable)[TEAM_MAX][FILE_MAX * RANK_MAX][FILE_MAX * RANK_MAX];
	};

	class BOX_API MoveSelector
	{
	private:
		const MoveOrderingInfo m_OrderingInfo;
		MoveList* m_LegalMoves;
		int m_CurrentIndex;

	public:
		MoveSelector(const MoveOrderingInfo& info, MoveList* legalMoves);
		
		bool Empty() const;
		Move GetNextMove();
	};

	class BOX_API QuiescenceMoveSelector
	{
	private:
		MoveList& m_LegalMoves;
		size_t m_CurrentIndex;
		size_t m_NumberOfCaptures;
		bool m_InCheck;

	public:
		QuiescenceMoveSelector(const Position& position, MoveList& legalMoves);

		bool Empty() const;
		Move GetNextMove();
	};

}
