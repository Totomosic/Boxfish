#pragma once
#include "Move.h"

namespace Boxfish
{

	class BOX_API MoveOrderer
	{
	public:
		std::vector<Move> OrderMoves(const std::vector<Move>& moves);
	};

}
