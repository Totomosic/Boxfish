#pragma once
#include "Move.h"
#include "MoveGenerator.h"

namespace Boxfish
{

	class BOX_API MoveSelector
	{
	private:
		MoveList& m_LegalMoves;
		int m_CurrentIndex;

	public:
		MoveSelector(MoveList& legalMoves);
		
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
