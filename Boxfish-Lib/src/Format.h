#pragma once
#include "MoveGenerator.h"

namespace Boxfish
{

	std::string SquareToAlgebraic(const Square& square);
	std::string SquareToAlgebraic(SquareIndex square);
	Square SquareFromAlgebraic(const std::string& square);
	SquareIndex SquareIndexFromAlgebraic(const std::string& square);

	class BOX_API UCI
	{
	public:
		UCI() = delete;

		static std::string FormatMove(const Move& move);
		static std::string FormatPV(const std::vector<Move>& moves);
		
		static Move CreateMoveFromString(const Position& position, const std::string& uciString);
	};

	class BOX_API PGN
	{
	public:
		PGN() = delete;

		// PGN formatting is context dependent so also needs the position to be specified
		// The position must be provided before the move is played
		static std::string FormatMove(const Move& move, const Position& position);

		static Move CreateMoveFromString(const Position& position, const std::string& pgnString);

	private:
		static char GetFileName(File file);
		static char GetRankName(Rank rank);
		static File GetFileFromName(char name);
		static Rank GetRankFromName(char name);
		static std::string GetPieceName(Piece piece);
		static Piece GetPieceFromName(char piece);
		static bool IsCapital(char c);
	};

}
