#include "Boxfish.h"

int main(int argc, const char** argv)
{
	Boxfish::Boxfish engine;
	Boxfish::Position position = Boxfish::CreateStartingPosition();
	engine.SetPosition(position);

	std::string version = "220620 64";

	std::cout << "Boxfish " << version << " by J. Morrison" << std::endl;

	std::cout << Boxfish::GetNonSlidingAttacks(Boxfish::PIECE_KING, { Boxfish::FILE_E, Boxfish::RANK_2 }, Boxfish::TEAM_WHITE) << std::endl;
	std::cout << Boxfish::GetSlidingAttacks(Boxfish::PIECE_QUEEN, { Boxfish::FILE_D, Boxfish::RANK_4 }, Boxfish::GetOverallPiecesBitBoard(engine.GetCurrentPosition())) << std::endl;

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
				engine.SetPosition(Boxfish::CreatePositionFromFEN(std::string(args.substr(isFen + 5))));
			}
		}
		else if (command == "moves")
		{
			Boxfish::MoveGenerator generator(engine.GetCurrentPosition());
			const std::vector<Boxfish::Move>& moves = generator.GetPseudoLegalMoves();
			for (const Boxfish::Move& move : moves)
			{
				std::cout << Boxfish::SquareToString(move.GetFromSquare()) << Boxfish::SquareToString(move.GetToSquare()) << std::endl;
			}
		}
		else
		{
			std::cout << "Unknown command: " << command << std::endl;
		}
	}

	return 0;
}
