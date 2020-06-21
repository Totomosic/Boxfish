#pragma once
#include <string>
#include "Types.h"

namespace Boxfish
{

	// Creates a square from a string such as "e4" -> { FILE_E, RANK_4 }
	inline Square SquareFromString(const std::string& square)
	{
		BOX_ASSERT(square.length() == 2, "Invalid square string");
		char file = square[0];
		char rank = square[1];
		return { (File)(file - 'a'), (Rank)(rank - '1') };
	}

	inline std::string SquareToString(const Square& square)
	{
		return std::string{ (char)((int)square.File + 'a'), (char)((int)square.Rank + '1') };
	}

}
