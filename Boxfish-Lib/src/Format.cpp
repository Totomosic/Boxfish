#include "Format.h"
#include "Attacks.h"
#include <fstream>

namespace Boxfish
{

	std::vector<std::string> Split(const std::string& str, const std::string& delimiter)
	{
		std::vector<std::string> result;
		size_t begin = 0;
		size_t end = str.find(delimiter, begin);
		while (end != std::string::npos)
		{
			result.push_back(str.substr(begin, end - begin));
			begin = end + delimiter.size();
			end = str.find(delimiter, begin);
		}
		result.push_back(str.substr(begin, end - begin));
		return result;
	}

	void CleanupString(std::string& str, const char* charsToRemove, int charCount)
	{
		while (true)
		{
			bool found = false;
			for (int i = 0; i < charCount; i++)
			{
				size_t pos = str.find(charsToRemove[i]);
				if (pos != std::string::npos)
				{
					found = true;
					str.erase(str.begin() + pos);
				}
			}
			if (!found)
				break;
		}
	}

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
			return MOVE_NONE;
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
		std::string moveString;
		if (move.GetFlags() & MOVE_KINGSIDE_CASTLE)
			moveString = "O-O";
		else if (move.GetFlags() & MOVE_QUEENSIDE_CASTLE)
			moveString = "O-O-O";
		else
		{
			moveString = GetPieceName(move.GetMovingPiece());
			BitBoard pieceAttacks = GetAttacksBy(move.GetMovingPiece(), move.GetToSquareIndex(), OtherTeam(position.TeamToPlay), position.GetAllPieces());
			BitBoard attacks = pieceAttacks & position.GetTeamPieces(position.TeamToPlay, move.GetMovingPiece());
			BitBoard pinnedAttacks = attacks & position.GetBlockersForKing(position.TeamToPlay);
			while (pinnedAttacks)
			{
				SquareIndex square = PopLeastSignificantBit(pinnedAttacks);
				if (!IsAligned(square, position.GetKingSquare(position.TeamToPlay), move.GetToSquareIndex()))
				{
					attacks &= ~square;
				}
			}
			BOX_ASSERT(attacks != ZERO_BB || move.GetMovingPiece() == PIECE_PAWN, "Invalid move");
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
		if (pgnString.substr(0, 5) == "O-O-O" || pgnString.substr(0, 5) == "0-0-0")
		{
			if (position.TeamToPlay == TEAM_WHITE)
				return CreateMove(position, { FILE_E, RANK_1 }, { FILE_C, RANK_1 });
			else
				return CreateMove(position, { FILE_E, RANK_8 }, { FILE_C, RANK_8 });
		}
		if (pgnString.substr(0, 3) == "O-O" || pgnString.substr(0, 3) == "0-0")
		{
			if (position.TeamToPlay == TEAM_WHITE)
				return CreateMove(position, { FILE_E, RANK_1 }, { FILE_G, RANK_1 });
			else
				return CreateMove(position, { FILE_E, RANK_8 }, { FILE_G, RANK_8 });
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
		if (equals != std::string::npos)
			endIndex -= 2;
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
			BitBoard pawns = position.GetTeamPieces(position.TeamToPlay, PIECE_PAWN) & FILE_MASKS[pawnFile] & ~InFrontOrEqual(toRank, position.TeamToPlay);
			move = CreateMove(position, BitBoard::BitIndexToSquare(FrontmostSquare(pawns, position.TeamToPlay)), BitBoard::BitIndexToSquare(toSquare), promotion);
		}
		else
		{
			BitBoard attacks = GetAttacksBy(movingPiece, toSquare, OtherTeam(position.TeamToPlay), position.GetAllPieces()) & (position.GetTeamPieces(position.TeamToPlay, movingPiece));
			BitBoard pinnedAttacks = attacks & position.GetBlockersForKing(position.TeamToPlay);
			while (pinnedAttacks)
			{
				SquareIndex square = PopLeastSignificantBit(pinnedAttacks);
				if (!IsAligned(square, position.GetKingSquare(position.TeamToPlay), toSquare))
				{
					attacks &= ~square;
				}
			}
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
		BOX_ASSERT(false, "Invalid PGN Move");
		return MOVE_NONE;
	}

	std::vector<PGNMatch> PGN::ReadFromString(const std::string& pgn)
	{
		enum ReadStage
		{
			TAGS,
			MOVES,
		};

		constexpr char CharsToRemove[] = { '\r' };
		constexpr char TagCharsToRemove[] = { '\\' };
		std::vector<PGNMatch> matches;
		std::vector<std::string> lines = Split(pgn, "\n");

		ReadStage currentStage = TAGS;
		PGNMatch currentMatch;
		Position currentPosition = CreateStartingPosition();
		currentMatch.InitialPosition = currentPosition;
		bool inComment = false;
		std::string result = "";

		for (std::string& line : lines)
		{
			CleanupString(line, CharsToRemove, sizeof(CharsToRemove) / sizeof(char));
			if (line.size() > 0)
			{
				if (line[0] == '[')
				{
					if (currentStage != TAGS)
					{
						// Start new Match
						matches.push_back(currentMatch);
						currentMatch = PGNMatch();
						currentStage = TAGS;
						currentPosition = CreateStartingPosition();
						inComment = false;
						currentMatch.InitialPosition = currentPosition;
						result = "";
					}
					size_t firstSpace = line.find_first_of(' ');
					if (firstSpace != std::string::npos)
					{
						std::string tagName = line.substr(1, firstSpace - 1);
						size_t firstQuote = line.find_first_of('"', firstSpace);
						size_t lastQuote = line.find_last_of('"');
						if (firstQuote != std::string::npos && firstQuote != lastQuote)
						{
							std::string tagValue = line.substr(firstQuote + 1, lastQuote - firstQuote - 1);
							CleanupString(tagValue, TagCharsToRemove, sizeof(TagCharsToRemove) / sizeof(char));
							currentMatch.Tags[tagName] = tagValue;

							if (tagName == "FEN")
							{
								currentPosition = CreatePositionFromFEN(tagValue);
								currentMatch.InitialPosition = currentPosition;
							}
							else if (tagName == "Result")
								result = tagValue;
							else if (tagName == "Variant")
								std::cout << tagValue << std::endl;
						}
						else
						{
							std::cout << "Invalid Tag Line: Missing start or end quote" << std::endl;
						}
					}
					else
					{
						std::cout << "Invalid Tag Line: Missing Space" << std::endl;
					}
				}
				if (currentStage == TAGS && std::isdigit(line[0]))
				{
					currentStage = MOVES;
				}
				if (currentStage == MOVES)
				{
					size_t current = 0;
					if (inComment)
					{
						current = line.find_first_of('}');
						if (current != std::string::npos)
							current++;
						else
							continue;
					}
					size_t end = line.find_first_of(' ', current);
					while (current != std::string::npos && current < line.size())
					{
						std::string str;
						if (end != std::string::npos)
							str = line.substr(current, end - current);
						else
							str = line.substr(current);
						if (str.size() > 0)
						{
							if (str[0] == '{')
							{
								inComment = true;
								end = line.find_first_of('}', current);
								if (end != std::string::npos)
									inComment = false;
							}
							else if (str[0] == ';' || str == result)
								break;
							else
							{
								if (str.find('.') == std::string::npos)
								{
									Move move = CreateMoveFromString(currentPosition, str);
									if (move != MOVE_NONE && IsLegalMoveSlow(currentPosition, move))
									{
										BOX_ASSERT(FormatMove(move, currentPosition) == str, "Invalid move");
										currentMatch.Moves.push_back(move);
										ApplyMove(currentPosition, move);
									}
									else
									{
										if (move == MOVE_NONE)
										{
											std::cout << currentPosition << std::endl;
											std::cout << "Side to Move: " << ((currentPosition.TeamToPlay == TEAM_WHITE) ? 'w' : 'b') << std::endl;
											std::cout << "Invalid move: " << str << std::endl;
										}
										else
										{
											std::cout << currentPosition << std::endl;
											std::cout << "Side to Move: " << ((currentPosition.TeamToPlay == TEAM_WHITE) ? 'w' : 'b') << std::endl;
											std::cout << "Illegal move: " << str << std::endl;
											std::cout << "Legal Moves: " << std::endl;
											for (const Move& move : GetLegalMovesSlow(currentPosition))
												std::cout << FormatMove(move, currentPosition) << std::endl;
										}
									}
								}
							}
						}
						if (end != std::string::npos)
						{
							current = end + 1;
							end = line.find_first_of(' ', current);
						}
						else
						{
							break;
						}
					}
				}
			}
		}
		matches.push_back(currentMatch);

		return matches;
	}

	std::vector<PGNMatch> PGN::ReadFromFile(const std::string& filename)
	{
		std::ifstream file(filename);
		file.seekg(0, std::ios::end);
		size_t filesize = file.tellg();
		file.seekg(0, std::ios::beg);
		std::string result;
		result.reserve(filesize / sizeof(char));
		result.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		file.close();
		return ReadFromString(result);
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
