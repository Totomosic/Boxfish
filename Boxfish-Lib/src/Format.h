#pragma once
#include "MoveGenerator.h"

namespace Boxfish
{

	std::vector<std::string> Split(const std::string& str, const std::string& delimiter);

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

	struct BOX_API PGNMatch
	{
	public:
		std::unordered_map<std::string, std::string> Tags;
		Position InitialPosition;
		std::vector<Move> Moves;
	};

	class BOX_API PGN
	{
	public:
		PGN() = delete;

		// PGN formatting is context dependent so also needs the position to be specified
		// The position must be provided before the move is played
		static std::string FormatMove(const Move& move, const Position& position);
		static Move CreateMoveFromString(const Position& position, const std::string& pgnString);

		static std::vector<PGNMatch> ReadFromString(const std::string& pgn);
		static std::vector<PGNMatch> ReadFromFile(const std::string& filename);

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
