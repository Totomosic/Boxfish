#pragma once
#include "MoveGenerator.h"
#include "Evaluation.h"
#include "PositionUtils.h"
#include "TranspositionTable.h"
#include "MoveSelector.h"

namespace Boxfish
{

	struct BOX_API Line
	{
	public:
		size_t CurrentIndex = 0;
		Move Moves[50];
	};

	std::string FormatLine(const Line& line, bool includeSymbols = true);

	class BOX_API Search
	{
	private:
		TranspositionTable m_TranspositionTable;
		Position m_CurrentPosition;

		Move m_BestMove;
		Centipawns m_BestScore;
		Line m_PV;

		size_t m_Nodes;

		bool m_Log;

	public:
		Search(bool log = true);

		const Move& GetBestMove() const;
		Centipawns GetBestScore() const;
		const Line& GetPV() const;

		void SetCurrentPosition(const Position& position);
		void SetCurrentPosition(Position&& position);
		void Go(int depth);
		void Reset();

	private:
		void SearchRoot(const Position& position, int depth);
		Centipawns Negamax(const Position& position, int depth, int alpha, int beta, Line& line);
		Centipawns QuiescenceSearch(const Position& position, int alpha, int beta);
	};

}
