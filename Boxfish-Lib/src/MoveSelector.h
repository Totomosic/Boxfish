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

	class BOX_API MoveSelector
	{
	public:
		enum SelectorStage
		{
			TTMove,
			Any
		};

	private:
		MoveList* m_Moves;
		const Position* m_CurrentPosition;
		Move m_ttMove;
		Move m_CounterMove;
		const OrderingTables* m_Tables;
		const Move* m_Killers;

		int m_CurrentIndex;
		SelectorStage m_CurrentStage;

	public:
		MoveSelector(MoveList* moves, const Position* currentPosition, Move ttMove, Move counterMove, Move prevMove, const OrderingTables* tables, const Move* killers);
		Move GetNextMove();
	};

	inline MoveSelector::SelectorStage operator++(MoveSelector::SelectorStage& stage, int)
	{
		MoveSelector::SelectorStage result = stage;
		stage = (MoveSelector::SelectorStage)(stage + 1);
		return result;
	}

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
