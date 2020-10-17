#include "Format.h"
#include "Attacks.h"

namespace Boxfish
{

	std::string SquareToAlgebraic(const Square& square)
	{
		return std::string{ (char)((int)square.File + 'a'), (char)((int)square.Rank + '1') };
	}

	std::string SquareToAlgebraic(SquareIndex square)
	{
		return SquareToAlgebraic(BitBoard::BitIndexToSquare(square));
	}

	Square SquareFromAlgebraic(const std::string& square)
	{
		BOX_ASSERT(square.length() == 2, "Invalid square string");
		char file = square[0];
		char rank = square[1];
		return { (File)(file - 'a'), (Rank)(rank - '1') };
	}

	SquareIndex SquareIndexFromAlgebraic(const std::string& square)
	{
		return BitBoard::SquareToBitIndex(SquareFromAlgebraic(square));
	}

	std::string UCI::FormatMove(const Move& move)
	{
		if (move.GetFlags() & MOVE_NULL)
			return "(none)";
		std::string result = SquareToAlgebraic(move.GetFromSquare()) + SquareToAlgebraic(move.GetToSquare());
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

	Move UCI::CreateMoveFromString(const Position& position, const std::string& uciString)
	{
		BOX_ASSERT(uciString.size() >= 4, "Invalid UCI move string");
		if (uciString.size() > 5)
		{
			BOX_WARN("Move string is too long {} characters, expected 4 or 5.", uciString.size());
		}
		File startFile = (File)(uciString[0] - 'a');
		Rank startRank = (Rank)(uciString[1] - '1');
		File endFile = (File)(uciString[2] - 'a');
		Rank endRank = (Rank)(uciString[3] - '1');
		BOX_ASSERT(startFile >= 0 && startFile < FILE_MAX && startRank >= 0 && startRank < RANK_MAX && endFile >= 0 && endFile < FILE_MAX && endRank >= 0 && endRank < RANK_MAX,
			"Invalid UCI move string. Ranks/Files out of range.");
		Piece promotion = PIECE_QUEEN;
		if (uciString.size() >= 5)
		{
			// support lower case or upper case
			char promotionChar = uciString[4];
			if (promotionChar - 'a' < 0)
				promotionChar += 'a' - 'A';
			switch (promotionChar)
			{
			case 'q':
				promotion = PIECE_QUEEN;
				break;
			case 'n':
				promotion = PIECE_KNIGHT;
				break;
			case 'r':
				promotion = PIECE_ROOK;
				break;
			case 'b':
				promotion = PIECE_BISHOP;
				break;
			default:
				BOX_ASSERT(false, "Invalid promotion type: {}", promotionChar);
				break;
			}
		}
		return CreateMove(position, { startFile, startRank }, { endFile, endRank }, promotion);
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
				moveString += SquareToAlgebraic(move.GetFromSquareIndex());
		}
		if (move.IsCapture())
		{
			if (move.GetMovingPiece() == PIECE_PAWN)
			{
				moveString += GetFileName(move.GetFromSquare().File);
			}
			moveString += 'x';
		}
		moveString += SquareToAlgebraic(move.GetToSquareIndex());
		if (move.IsPromotion())
		{
			moveString += "=" + GetPieceName(move.GetPromotionPiece());
		}
		Position pos = position;
		ApplyMove(pos, move);
		if (pos.InCheck())
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

	Move PGN::CreateMoveFromString(const Position& position, const std::string& pgnString)
	{
		Move move = MOVE_NONE;
		if (pgnString == "O-O")
		{
			if (position.TeamToPlay == TEAM_WHITE)
				return CreateMove(position, { FILE_E, RANK_1 }, { FILE_G, RANK_1 });
			else
				return CreateMove(position, { FILE_E, RANK_8 }, { FILE_G, RANK_8 });
		}
		if (pgnString == "O-O-O")
		{
			if (position.TeamToPlay == TEAM_WHITE)
				return CreateMove(position, { FILE_E, RANK_1 }, { FILE_C, RANK_1 });
			else
				return CreateMove(position, { FILE_E, RANK_8 }, { FILE_C, RANK_8 });
		}
		size_t equals = pgnString.find('=');
		Piece promotion = PIECE_INVALID;
		if (equals != std::string::npos)
		{
			promotion = GetPieceFromName(pgnString[equals + 1]);
		}
		size_t endIndex = pgnString.size() - 1;
		if (pgnString[endIndex] == '#' || pgnString[endIndex] == '+')
			endIndex--;
		SquareIndex toSquare = SquareIndexFromAlgebraic(pgnString.substr(endIndex - 1, 2));
		Piece movingPiece = PIECE_PAWN;
		File pawnFile = FILE_INVALID;

		bool isCapture = pgnString.find('x') != std::string::npos;
		
		if (IsCapital(pgnString[0]))
			movingPiece = GetPieceFromName(pgnString[0]);
		else
		{
			pawnFile = GetFileFromName(pgnString[0]);
		}

		if (movingPiece == PIECE_PAWN && !isCapture)
		{
			Rank toRank = BitBoard::RankOfIndex(toSquare);
			BitBoard pawns = position.GetTeamPieces(position.TeamToPlay, PIECE_PAWN) & FILE_MASKS[pawnFile] & ~InFrontOrEqual(Rank(toRank - 1), position.TeamToPlay);
			move = CreateMove(position, BitBoard::BitIndexToSquare(FrontmostSquare(pawns, position.TeamToPlay)), BitBoard::BitIndexToSquare(toSquare), promotion);
		}
		else
		{
			BitBoard attacks = GetAttacksBy(movingPiece, toSquare, OtherTeam(position.TeamToPlay), position.GetAllPieces()) & position.GetTeamPieces(position.TeamToPlay, movingPiece);
			if (movingPiece == PIECE_PAWN && pawnFile != FILE_INVALID)
				attacks &= FILE_MASKS[pawnFile];
			if (attacks == ZERO_BB)
				return MOVE_NONE;
			if (MoreThanOne(attacks))
			{
				SquareIndex fromSquare;
				char disambiguation = pgnString[1];
				if (std::isdigit(disambiguation))
				{
					// Rank
					fromSquare = BackwardBitScan(attacks & RANK_MASKS[GetRankFromName(disambiguation)]);
				}
				else
				{
					// File/Square
					attacks = attacks & FILE_MASKS[GetFileFromName(disambiguation)];
					if (MoreThanOne(attacks))
					{
						attacks = attacks & RANK_MASKS[GetRankFromName(pgnString[2])];
					}
					fromSquare = BackwardBitScan(attacks);
				}
				move = CreateMove(position, BitBoard::BitIndexToSquare(fromSquare), BitBoard::BitIndexToSquare(toSquare), promotion);
			}
			else
			{
				move = CreateMove(position, BitBoard::BitIndexToSquare(BackwardBitScan(attacks)), BitBoard::BitIndexToSquare(toSquare), promotion);
			}
		}
		if (move.IsCapture() == isCapture && (promotion != PIECE_INVALID) == move.IsPromotion())
			return move;
		return MOVE_NONE;
	}

	char PGN::GetFileName(File file)
	{
		return (char)('a' + (file - FILE_A));
	}

	char PGN::GetRankName(Rank rank)
	{
		return (char)('1' + (rank - RANK_1));
	}

	File PGN::GetFileFromName(char name)
	{
		return (File)(name - 'a' + FILE_A);
	}

	Rank PGN::GetRankFromName(char name)
	{
		return (Rank)(name - '1' + RANK_1);
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

	Piece PGN::GetPieceFromName(char piece)
	{
		switch (piece)
		{
		case 'N':
			return PIECE_KNIGHT;
		case 'B':
			return PIECE_BISHOP;
		case 'R':
			return PIECE_ROOK;
		case 'Q':
			return PIECE_QUEEN;
		case 'K':
			return PIECE_KING;
		default:
			return PIECE_INVALID;
		}
		return PIECE_INVALID;
	}

	bool PGN::IsCapital(char c)
	{
		return std::isupper(c);
	}

}
