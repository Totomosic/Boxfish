#include "MoveOrdering.h"

namespace Boxfish
{

	int ScoreMove(const Move& move)
	{
		int value = 0;
		if (move.GetFlags() & MOVE_CAPTURE)
		{
			value = 5;
		}
		return value;
	}

	std::vector<Move> MoveOrderer::OrderMoves(const std::vector<Move>& moves)
	{
		std::vector<Move> result = moves;
		std::sort(result.begin(), result.end(), [](const Move& a, const Move& b)
			{
				return ScoreMove(a) > ScoreMove(b);
			});
		return result;
	}

}
