#include "Format.h"
#include "Attacks.h"

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

	std::string PGN::FormatMove(const Move& move, const Position& position)
	{
		if (move.GetFlags() & MOVE_KINGSIDE_CASTLE)
			return "O-O";
		if (move.GetFlags() & MOVE_QUEENSIDE_CASTLE)
			return "O-O-O";

		std::string moveString = GetPieceName(move.GetMovingPiece());
		BitBoard attacks = GetAttacksBy(move.GetMovingPiece(), move.GetToSquareIndex(), position.TeamToPlay, position.GetAllPieces()) & position.GetTeamPieces(position.TeamToPlay, move.GetMovingPiece());
		BOX_ASSERT(attacks != ZERO_BB, "Invalid move");
		if (MoreThanOne(attacks) && move.GetMovingPiece() != PIECE_PAWN)
		{
			// Resolve ambiguity
			if (!MoreThanOne(attacks & FILE_MASKS[move.GetFromSquare().File]))
				moveString += GetFileName(move.GetFromSquare().File);
			else if (!MoreThanOne(attacks & RANK_MASKS[move.GetFromSquare().Rank]))
				moveString += GetRankName(move.GetFromSquare().Rank);
			else
				moveString += UCI::FormatSquare(move.GetFromSquareIndex());
		}
		if (move.IsCapture())
		{
			if (move.GetMovingPiece() == PIECE_PAWN)
			{
				moveString += GetFileName(move.GetFromSquare().File);
			}
			moveString += 'x';
		}
		moveString += UCI::FormatSquare(move.GetToSquareIndex());
		if (move.IsPromotion())
		{
			moveString += "=" + GetPieceName(move.GetPromotionPiece());
		}
		Position pos = position;
		ApplyMove(pos, move);
		if (IsInCheck(pos))
		{
			Move buffer[MAX_MOVES];
			MoveGenerator generator(pos);
			MoveList moves(buffer);
			generator.GetPseudoLegalMoves(moves);
			generator.FilterLegalMoves(moves);
			moveString += moves.MoveCount == 0 ? '#' : '+';
		}
		return moveString;
	}

	char PGN::GetFileName(File file)
	{
		return (char)('a' + (file - FILE_A));
	}

	char PGN::GetRankName(Rank rank)
	{
		return (char)('1' + (rank - RANK_1));
	}

	std::string PGN::GetPieceName(Piece piece)
	{
		switch (piece)
		{
		case PIECE_PAWN:
			return "";
		case PIECE_KNIGHT:
			return "N";
		case PIECE_BISHOP:
			return "B";
		case PIECE_ROOK:
			return "R";
		case PIECE_QUEEN:
			return "Q";
		case PIECE_KING:
			return "K";
		default:
			break;
		}
		BOX_ASSERT(false, "Invalid piece");
		return "";
	}

}
