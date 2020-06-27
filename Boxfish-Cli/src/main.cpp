#include "Boxfish.h"

using namespace Boxfish;

int main(int argc, const char** argv)
{
	::Boxfish::Boxfish engine;
	Position position = CreateStartingPosition();
	engine.SetPosition(position);

	std::string version = __TIMESTAMP__;

	std::cout << "Boxfish " << version << " by J. Morrison" << std::endl;

	// DEBUG FEN: 1B2k2r/1P1n3p/5pp1/1Q6/3Pp3/4P3/7P/4KNq1 b k - 0 37
	// Can't move because in check

	char buffer[4096];
	while (true)
	{
		std::cin.getline(buffer, sizeof(buffer));
		std::string_view line = buffer;
		std::string_view command;
		std::string_view args;

		size_t space = line.find_first_of(' ');
		if (space != std::string_view::npos)
		{
			command = line.substr(0, space);
			args = line.substr(space + 1);
		}
		else
		{
			command = line;
		}
		
		if (command == "d")
		{
			std::cout << std::endl << engine.GetCurrentPosition() << std::endl;
			std::cout << std::endl << "Fen: " << GetFENFromPosition(engine.GetCurrentPosition()) << std::endl;
			std::cout << std::hex << "Hash: " << engine.GetCurrentPosition().Hash.Hash << std::dec << std::endl;
		}
		else if (command == "isready")
		{
			std::cout << "readyok" << std::endl;
		}
		else if (command == "position")
		{
			size_t isFen = args.find("fen ");
			if (isFen != std::string_view::npos)
			{
				engine.SetPosition(CreatePositionFromFEN(std::string(args.substr(isFen + 4))));
			}
			size_t startPos = args.find("startpos");
			if (startPos != std::string_view::npos)
			{
				engine.SetPosition(CreateStartingPosition());
			}
			size_t moves = args.find("moves ");
			if (moves != std::string_view::npos)
			{
				size_t begin = moves + 6;
				while (begin != std::string_view::npos && begin < args.size())
				{
					size_t end = args.find(' ', begin);
					std::string_view move = args.substr(begin, end - begin);
					if (move.size() == 4)
					{
						std::string from = std::string(move.substr(0, 2));
						std::string to = std::string(move.substr(2));
						Square fromSquare = SquareFromString(from);
						Square toSquare = SquareFromString(to);
						Position position = engine.GetCurrentPosition();
						Move move = CreateMove(position, fromSquare, toSquare);
						if (SanityCheckMove(position, move))
						{
							ApplyMove(position, move);
							engine.SetPosition(position);
						}
						else
						{
							std::cout << "Move is not valid." << std::endl;
						}
					}
					else
					{
						BOX_WARN("Invalid move length");
						break;
					}
					if (end != std::string_view::npos)
						begin = args.find_first_not_of(' ', end + 1);
					else
						begin = end;
				}
			}
		}
		else if (command == "go")
		{
			size_t depthString = args.find("depth ");
			if (depthString != std::string_view::npos)
			{
				size_t space = args.find_first_of(' ', depthString + 6);
				int depth = std::stoi(std::string(args.substr(depthString + 6, space - depthString - 6)));
				Search& search = engine.GetSearch();
				search.SetCurrentPosition(engine.GetCurrentPosition());
				search.Go(depth);
				std::cout << "bestmove " << FormatMove(search.GetBestMove(), false) << " " << search.GetBestScore() << std::endl;
			}
		}
		else if (command == "eval")
		{
			std::cout << Evaluate(engine.GetCurrentPosition(), TEAM_WHITE) << std::endl;
		}
		else if (command == "moves")
		{
			MoveGenerator generator(engine.GetCurrentPosition());
			const std::vector<Move>& moves = generator.GetLegalMoves();
			if (moves.size() > 0)
			{
				for (const Move& move : moves)
				{
					std::cout << FormatMove(move) << std::endl;
				}
			}
			else
			{
				std::cout << FormatNullMove() << std::endl;
			}
		}
		else
		{
			std::cout << "Unknown command: " << command << std::endl;
		}
	}

	return 0;
}
