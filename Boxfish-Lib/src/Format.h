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

	class BOX_API PGN
	{
	public:
		PGN() = delete;

		// PGN formatting is context dependent so also needs the position to be specified
		// The position must be provided before the move is played
		static std::string FormatMove(const Move& move, const Position& position);

	private:
		static char GetFileName(File file);
		static char GetRankName(Rank rank);
		static std::string GetPieceName(Piece piece);
	};

}
