#include "PositionUtils.h"
#include "Utils.h"

namespace Boxfish
{

	static char PieceToFEN(Piece piece, bool isWhite)
	{
		switch (piece)
		{
		case PIECE_PAWN:
			return (isWhite) ? 'P' : 'p';
		case PIECE_KNIGHT:
			return (isWhite) ? 'N' : 'n';
		case PIECE_BISHOP:
			return (isWhite) ? 'B' : 'b';
		case PIECE_ROOK:
			return (isWhite) ? 'R' : 'r';
		case PIECE_QUEEN:
			return (isWhite) ? 'Q' : 'q';
		case PIECE_KING:
			return (isWhite) ? 'K' : 'k';
		}
		BOX_ASSERT(false, "Invalid piece");
		return ' ';
	}

	Position CreateStartingPosition()
	{
		Position result;
		result.Teams[TEAM_WHITE].Pieces[PIECE_PAWN].SetAt({ FILE_A, RANK_2 });
		result.Teams[TEAM_WHITE].Pieces[PIECE_PAWN].SetAt({ FILE_B, RANK_2 });
		result.Teams[TEAM_WHITE].Pieces[PIECE_PAWN].SetAt({ FILE_C, RANK_2 });
		result.Teams[TEAM_WHITE].Pieces[PIECE_PAWN].SetAt({ FILE_D, RANK_2 });
		result.Teams[TEAM_WHITE].Pieces[PIECE_PAWN].SetAt({ FILE_E, RANK_2 });
		result.Teams[TEAM_WHITE].Pieces[PIECE_PAWN].SetAt({ FILE_F, RANK_2 });
		result.Teams[TEAM_WHITE].Pieces[PIECE_PAWN].SetAt({ FILE_G, RANK_2 });
		result.Teams[TEAM_WHITE].Pieces[PIECE_PAWN].SetAt({ FILE_H, RANK_2 });

		result.Teams[TEAM_WHITE].Pieces[PIECE_ROOK].SetAt({ FILE_A, RANK_1 });
		result.Teams[TEAM_WHITE].Pieces[PIECE_KNIGHT].SetAt({ FILE_B, RANK_1 });
		result.Teams[TEAM_WHITE].Pieces[PIECE_BISHOP].SetAt({ FILE_C, RANK_1 });
		result.Teams[TEAM_WHITE].Pieces[PIECE_QUEEN].SetAt({ FILE_D, RANK_1 });
		result.Teams[TEAM_WHITE].Pieces[PIECE_KING].SetAt({ FILE_E, RANK_1 });
		result.Teams[TEAM_WHITE].Pieces[PIECE_BISHOP].SetAt({ FILE_F, RANK_1 });
		result.Teams[TEAM_WHITE].Pieces[PIECE_KNIGHT].SetAt({ FILE_G, RANK_1 });
		result.Teams[TEAM_WHITE].Pieces[PIECE_ROOK].SetAt({ FILE_H, RANK_1 });

		result.Teams[TEAM_BLACK].Pieces[PIECE_PAWN].SetAt({ FILE_A, RANK_7 });
		result.Teams[TEAM_BLACK].Pieces[PIECE_PAWN].SetAt({ FILE_B, RANK_7 });
		result.Teams[TEAM_BLACK].Pieces[PIECE_PAWN].SetAt({ FILE_C, RANK_7 });
		result.Teams[TEAM_BLACK].Pieces[PIECE_PAWN].SetAt({ FILE_D, RANK_7 });
		result.Teams[TEAM_BLACK].Pieces[PIECE_PAWN].SetAt({ FILE_E, RANK_7 });
		result.Teams[TEAM_BLACK].Pieces[PIECE_PAWN].SetAt({ FILE_F, RANK_7 });
		result.Teams[TEAM_BLACK].Pieces[PIECE_PAWN].SetAt({ FILE_G, RANK_7 });
		result.Teams[TEAM_BLACK].Pieces[PIECE_PAWN].SetAt({ FILE_H, RANK_7 });

		result.Teams[TEAM_BLACK].Pieces[PIECE_ROOK].SetAt({ FILE_A, RANK_8 });
		result.Teams[TEAM_BLACK].Pieces[PIECE_KNIGHT].SetAt({ FILE_B, RANK_8 });
		result.Teams[TEAM_BLACK].Pieces[PIECE_BISHOP].SetAt({ FILE_C, RANK_8 });
		result.Teams[TEAM_BLACK].Pieces[PIECE_QUEEN].SetAt({ FILE_D, RANK_8 });
		result.Teams[TEAM_BLACK].Pieces[PIECE_KING].SetAt({ FILE_E, RANK_8 });
		result.Teams[TEAM_BLACK].Pieces[PIECE_BISHOP].SetAt({ FILE_F, RANK_8 });
		result.Teams[TEAM_BLACK].Pieces[PIECE_KNIGHT].SetAt({ FILE_G, RANK_8 });
		result.Teams[TEAM_BLACK].Pieces[PIECE_ROOK].SetAt({ FILE_H, RANK_8 });
		return result;
	}

	Position CreatePositionFromFEN(const std::string& fen)
	{
		Position position;
		File currentFile = FILE_A;
		Rank currentRank = RANK_8;
		int index = 0;
		for (char c : fen)
		{
			index++;
			if (c == ' ')
				break;
			if (std::isdigit(c))
			{
				int count = c - '0';
				currentFile = (File)((int)currentFile + count);
			}
			if (c == '/')
			{
				currentRank--;
				currentFile = FILE_A;
			}
			switch (c)
			{
			case 'P':
				position.Teams[TEAM_WHITE].Pieces[PIECE_PAWN].SetAt({ currentFile++, currentRank });
				break;
			case 'N':
				position.Teams[TEAM_WHITE].Pieces[PIECE_KNIGHT].SetAt({ currentFile++, currentRank });
				break;
			case 'B':
				position.Teams[TEAM_WHITE].Pieces[PIECE_BISHOP].SetAt({ currentFile++, currentRank });
				break;
			case 'R':
				position.Teams[TEAM_WHITE].Pieces[PIECE_ROOK].SetAt({ currentFile++, currentRank });
				break;
			case 'Q':
				position.Teams[TEAM_WHITE].Pieces[PIECE_QUEEN].SetAt({ currentFile++, currentRank });
				break;
			case 'K':
				position.Teams[TEAM_WHITE].Pieces[PIECE_KING].SetAt({ currentFile++, currentRank });
				break;
			case 'p':
				position.Teams[TEAM_BLACK].Pieces[PIECE_PAWN].SetAt({ currentFile++, currentRank });
				break;
			case 'n':
				position.Teams[TEAM_BLACK].Pieces[PIECE_KNIGHT].SetAt({ currentFile++, currentRank });
				break;
			case 'b':
				position.Teams[TEAM_BLACK].Pieces[PIECE_BISHOP].SetAt({ currentFile++, currentRank });
				break;
			case 'r':
				position.Teams[TEAM_BLACK].Pieces[PIECE_ROOK].SetAt({ currentFile++, currentRank });
				break;
			case 'q':
				position.Teams[TEAM_BLACK].Pieces[PIECE_QUEEN].SetAt({ currentFile++, currentRank });
				break;
			case 'k':
				position.Teams[TEAM_BLACK].Pieces[PIECE_KING].SetAt({ currentFile++, currentRank });
				break;
			}
		}
		position.TeamToPlay = (fen[index] == 'w') ? TEAM_WHITE : TEAM_BLACK;
		index += 2;

		position.Teams[TEAM_WHITE].CastleKingSide = false;
		position.Teams[TEAM_WHITE].CastleQueenSide = false;
		position.Teams[TEAM_BLACK].CastleKingSide = false;
		position.Teams[TEAM_BLACK].CastleQueenSide = false;

		while (fen[index] != ' ')
		{
			switch (fen[index])
			{
			case 'K':
				position.Teams[TEAM_WHITE].CastleKingSide = true;
				break;
			case 'Q':
				position.Teams[TEAM_WHITE].CastleQueenSide = true;
				break;
			case 'k':
				position.Teams[TEAM_BLACK].CastleKingSide = true;
				break;
			case 'q':
				position.Teams[TEAM_BLACK].CastleQueenSide = true;
				break;
			}
			index++;
		}
		index++;

		if (fen[index] != '-')
		{
			position.EnpassantSquare = SquareFromString(fen.substr(index, 2));
			index += 3;
		}
		else
		{
			index += 2;
		}

		size_t space = fen.find_first_of(' ', index);
		position.HalfTurnsSinceCaptureOrPush = std::stoi(fen.substr(index, space - index));
		index = space + 1;
		space = fen.find_first_of(' ', index);
		position.TotalTurns = std::stoi(fen.substr(index)) - 1;
	
		return position;
	}

	std::string GetFENFromPosition(const Position& position)
	{
		std::string result = "";
		BitBoard overall;

		for (Rank rank = RANK_8; rank >= RANK_1; rank--)
		{
			int emptyCount = 0;
			for (File file = FILE_A; file < FILE_MAX; file++)
			{
				char pieceFEN = 0;
				for (Piece piece = PIECE_PAWN; piece < PIECE_MAX; piece++)
				{
					if (position.Teams[TEAM_WHITE].Pieces[piece].GetAt({ file, rank }))
					{
						pieceFEN = PieceToFEN(piece, true);
						break;
					}
					if (position.Teams[TEAM_BLACK].Pieces[piece].GetAt({ file, rank }))
					{
						pieceFEN = PieceToFEN(piece, false);
						break;
					}
				}
				if (pieceFEN != 0)
				{
					if (emptyCount > 0)
					{
						result += (char)(emptyCount + '0');
						emptyCount = 0;
					}
					result += pieceFEN;
				}
				else
				{
					emptyCount++;
				}
			}
			if (emptyCount > 0)
			{
				result += (char)(emptyCount + '0');
				emptyCount = 0;
			}
			if (rank != RANK_1)
				result += '/';
		}

		result += (position.TeamToPlay == TEAM_WHITE) ? " w" : " b";
		result += ' ';
		if (position.Teams[TEAM_WHITE].CastleKingSide || position.Teams[TEAM_WHITE].CastleQueenSide || position.Teams[TEAM_BLACK].CastleKingSide || position.Teams[TEAM_BLACK].CastleQueenSide)
		{
			if (position.Teams[TEAM_WHITE].CastleKingSide)
				result += 'K';
			if (position.Teams[TEAM_WHITE].CastleQueenSide)
				result += 'Q';
			if (position.Teams[TEAM_BLACK].CastleKingSide)
				result += 'k';
			if (position.Teams[TEAM_BLACK].CastleQueenSide)
				result += 'q';
		}
		else
		{
			result += '-';
		}

		if (position.EnpassantSquare == INVALID_SQUARE)
			result += " -";
		else
			result += " " + SquareToString(position.EnpassantSquare);

		result += " " + std::to_string(position.HalfTurnsSinceCaptureOrPush);
		result += " " + std::to_string(position.TotalTurns + 1);

		return result;
	}

	BitBoard GetTeamPiecesBitBoard(const Position& position, Team team)
	{
		BitBoard result = 0;
		for (Piece piece = PIECE_PAWN; piece < PIECE_MAX; piece++)
			result |= position.Teams[team].Pieces[piece];
		return result;
	}

	BitBoard GetOverallPiecesBitBoard(const Position& position)
	{
		return GetTeamPiecesBitBoard(position, TEAM_WHITE) | GetTeamPiecesBitBoard(position, TEAM_BLACK);
	}

	Piece GetPieceAtSquare(const Position& position, Team team, const Square& square)
	{
		BitBoard squareBoard(1ULL << BitBoard::SquareToBitIndex(square));
		if (squareBoard & position.Teams[team].Pieces[PIECE_PAWN])
			return PIECE_PAWN;
		if (squareBoard & position.Teams[team].Pieces[PIECE_KNIGHT])
			return PIECE_KNIGHT;
		if (squareBoard & position.Teams[team].Pieces[PIECE_BISHOP])
			return PIECE_BISHOP;
		if (squareBoard & position.Teams[team].Pieces[PIECE_ROOK])
			return PIECE_ROOK;
		if (squareBoard & position.Teams[team].Pieces[PIECE_QUEEN])
			return PIECE_QUEEN;
		if (squareBoard & position.Teams[team].Pieces[PIECE_KING])
			return PIECE_KING;
		BOX_ASSERT(false, "No piece found");
		return PIECE_PAWN;
	}

	std::ostream& operator<<(std::ostream& stream, const Position& position)
	{
		for (Rank rank = RANK_8; rank >= RANK_1; rank--)
		{
			stream << ' ' << '+';
			for (Rank rank = RANK_8; rank >= RANK_1; rank--)
			{
				stream << "---+";
			}
			stream << std::endl << ' ' << '|';
			for (File file = FILE_A; file < FILE_MAX; file++)
			{
				char pieceFen = ' ';
				for (Piece piece = PIECE_PAWN; piece < PIECE_MAX; piece++)
				{
					if (position.Teams[TEAM_WHITE].Pieces[piece].GetAt({ file, rank }))
					{
						pieceFen = PieceToFEN(piece, true);
						break;
					}
					if (position.Teams[TEAM_BLACK].Pieces[piece].GetAt({ file, rank }))
					{
						pieceFen = PieceToFEN(piece, false);
						break;
					}
				}
				stream << ' ' << pieceFen << ' ' << '|';
			}
			stream << std::endl;
		}
		stream << ' ' << '+';
		for (Rank rank = RANK_8; rank >= RANK_1; rank--)
		{
			stream << "---+";
		}
		return stream;
	}

}
