#include "Boxfish.h"

int main(int argc, const char** argv)
{
	Boxfish::Boxfish engine;
	Boxfish::Position position = Boxfish::CreateStartingPosition();
	engine.SetPosition(position);

	std::string version = __TIMESTAMP__;

	std::cout << "Boxfish " << version << " by J. Morrison" << std::endl;

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
			std::cout << std::endl << "Fen: " << Boxfish::GetFENFromPosition(engine.GetCurrentPosition()) << std::endl;
			std::cout << std::hex << "Hash: " << engine.GetCurrentPosition().Hash.Hash << std::endl;
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
				engine.SetPosition(Boxfish::CreatePositionFromFEN(std::string(args.substr(isFen + 4))));
			}
			size_t startPos = args.find("startpos");
			if (startPos != std::string_view::npos)
			{
				engine.SetPosition(Boxfish::CreateStartingPosition());
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
						Boxfish::Square fromSquare = Boxfish::SquareFromString(from);
						Boxfish::Square toSquare = Boxfish::SquareFromString(to);
						Boxfish::Position position = engine.GetCurrentPosition();
						Boxfish::ApplyMove(position, Boxfish::CreateMove(position, fromSquare, toSquare));
						engine.SetPosition(position);
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
		else if (command == "eval")
		{
			std::cout << Boxfish::Evaluate(engine.GetCurrentPosition(), Boxfish::TEAM_WHITE) << std::endl;
		}
		else if (command == "moves")
		{
			Boxfish::MoveGenerator generator(engine.GetCurrentPosition());
			const std::vector<Boxfish::Move>& moves = generator.GetLegalMoves();
			if (moves.size() > 0)
			{
				for (const Boxfish::Move& move : moves)
				{
					std::cout << Boxfish::FormatMove(move) << std::endl;
				}
			}
			else
			{
				std::cout << Boxfish::FormatNullMove() << std::endl;
			}
		}
		else
		{
			std::cout << "Unknown command: " << command << std::endl;
		}
	}

	return 0;
}
