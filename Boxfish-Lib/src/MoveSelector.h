#pragma once
#include "Move.h"
#include "MoveGenerator.h"
#include "Evaluation.h"

namespace Boxfish
{

	struct BOX_API OrderingTables
	{
	public:
		Move CounterMoves[SQUARE_MAX][SQUARE_MAX];
		Centipawns History[TEAM_MAX][SQUARE_MAX][SQUARE_MAX];
		Centipawns Butterfly[TEAM_MAX][SQUARE_MAX][SQUARE_MAX];

	public:
		inline void Clear()
		{
			for (int i = 0; i < SQUARE_MAX; i++)
			{
				for (int j = 0; j < SQUARE_MAX; j++)
				{
					CounterMoves[i][j] = MOVE_NONE;
					History[TEAM_WHITE][i][j] = 0;
					History[TEAM_BLACK][i][j] = 0;
					Butterfly[TEAM_WHITE][i][j] = 0;
					Butterfly[TEAM_BLACK][i][j] = 0;
				}
			}
		}
	};

	struct BOX_API MoveOrderingInfo
	{
	public:
		const Position* CurrentPosition = nullptr;
		const Move* KillerMoves = nullptr;
		Move PreviousMove = MOVE_NONE;
		const OrderingTables* Tables = nullptr;
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
