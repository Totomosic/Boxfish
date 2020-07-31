#include "Uci.h"

namespace Boxfish
{

	std::string UCI::FormatSquare(const Square& square)
	{
		return std::string{ (char)('a' + square.File), (char)('1' + square.Rank) };
	}

	std::string UCI::FormatSquare(SquareIndex square)
	{
		return FormatSquare(BitBoard::BitIndexToSquare(square));
	}

	std::string UCI::FormatMove(const Move& move)
	{
		if (move.GetFlags() & MOVE_NULL)
			return "(none)";
		std::string result = FormatSquare(move.GetFromSquare()) + FormatSquare(move.GetToSquare());
		if (move.GetFlags() & MOVE_PROMOTION)
		{
			Piece promotion = move.GetPromotionPiece();
			switch (promotion)
			{
			case PIECE_QUEEN:
				result += 'q';
				break;
			case PIECE_ROOK:
				result += 'r';
				break;
			case PIECE_BISHOP:
				result += 'b';
				break;
			case PIECE_KNIGHT:
				result += 'n';
				break;
			}
		}
		return result;
	}

	std::string UCI::FormatPV(const std::vector<Move>& moves)
	{
		if (moves.size() > 0)
		{
			std::string result = FormatMove(moves[0]);
			for (int i = 1; i < moves.size(); i++)
				result += " " + FormatMove(moves[i]);
			return result;
		}
		return "";
	}

}
