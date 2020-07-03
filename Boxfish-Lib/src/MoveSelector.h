#pragma once
#include "Move.h"
#include "MoveGenerator.h"
#include "Evaluation.h"

namespace Boxfish
{

	struct BOX_API Line
	{
	public:
		size_t CurrentIndex = 0;
		Move Moves[50];
	};

	std::string FormatLine(const Line& line, bool includeSymbols = true);

	struct BOX_API MoveOrderingInfo
	{
	public:
		const Position* CurrentPosition = nullptr;
		Move PVMove;
		std::function<Centipawns(const Position&, const Move&)> MoveEvaluator;
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
		int m_MoveScores[MAX_MOVES];
		size_t m_CurrentIndex;
		size_t m_NumberOfCaptures;

	public:
		QuiescenceMoveSelector(const Position& position, MoveList& legalMoves);

		bool Empty() const;
		Move GetNextMove();
	};

}
