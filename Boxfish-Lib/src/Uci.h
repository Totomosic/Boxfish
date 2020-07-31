#pragma once
#include "MoveGenerator.h"

namespace Boxfish
{

	class BOX_API UCI
	{
	public:
		UCI() = delete;

		static std::string FormatSquare(const Square& square);
		static std::string FormatSquare(SquareIndex square);
		static std::string FormatMove(const Move& move);
		static std::string FormatPV(const std::vector<Move>& moves);
	};

}
