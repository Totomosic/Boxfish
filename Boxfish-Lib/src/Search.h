#pragma once
#include "MoveGenerator.h"
#include "Evaluation.h"
#include "PositionUtils.h"
#include "TranspositionTable.h"
#include "MoveOrdering.h"

namespace Boxfish
{

	class BOX_API Search
	{
	private:
		TranspositionTable m_TranspositionTable;
		Position m_CurrentPosition;

		Move m_BestMove;
		Centipawns m_BestScore;

		size_t m_Nodes;

		bool m_Log;

	public:
		Search(bool log = true);

		const Move& GetBestMove() const;
		Centipawns GetBestScore() const;

		void SetCurrentPosition(const Position& position);
		void SetCurrentPosition(Position&& position);
		void Go(int depth);
		void Reset();

	private:
		void StartSearch(const Position& position, int depth);
		Centipawns Negamax(const Position& position, int depth, int alpha, int beta);
	};

}
