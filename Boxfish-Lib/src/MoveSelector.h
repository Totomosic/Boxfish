#pragma once
#include "Move.h"
#include <random>

namespace Boxfish
{

	class BOX_API MoveSelector
	{
	private:
		std::vector<Move>& m_LegalMoves;

		std::mt19937 m_RandEngine;

	public:
		MoveSelector(std::vector<Move>& legalMoves);
		
		bool Empty() const;
		Move GetNextMove();
	};

}
