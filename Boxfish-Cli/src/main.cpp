#include "Boxfish.h"

using namespace Boxfish;

uint64_t Perft(Position& position, int depth)
{
	if (depth <= 0)
		return 1;
	if (depth == 1)
	{
		MoveGenerator movegen(position);
		MoveList moves = movegen.GetPseudoLegalMoves();
		movegen.FilterLegalMoves(moves);
		return moves.MoveCount;
	}

	MoveGenerator movegen(position);
	uint64_t nodes = 0;
	MoveList legalMoves = movegen.GetPseudoLegalMoves();
	movegen.FilterLegalMoves(legalMoves);
	for (int i = 0; i < legalMoves.MoveCount; i++)
	{
		Position movedPosition = position;
		ApplyMove(movedPosition, legalMoves.Moves[i]);
		nodes += Perft(movedPosition, depth - 1);
	}
	return nodes;
}

uint64_t PerftRoot(Position& position, int depth)
{
	uint64_t total = 0;
	MoveGenerator movegen(position);
	MoveList moves = movegen.GetPseudoLegalMoves();
	movegen.FilterLegalMoves(moves);

	auto startTime = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < moves.MoveCount; i++)
	{
		Position movedPosition = position;
		ApplyMove(movedPosition, moves.Moves[i]);
		uint64_t perft = Perft(movedPosition, depth - 1);
		total += perft;

		std::cout << FormatMove(moves.Moves[i], false) << ": " << perft << std::endl;
	}
	auto endTime = std::chrono::high_resolution_clock::now();
	auto elapsed = endTime - startTime;
	std::cout << "Total Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() << "ms" << std::endl;
	std::cout << "Total Nodes: " << total << std::endl;
	std::cout << "Nodes per Second: " << (size_t)(total / (double)std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count() * 1e9) << std::endl;
	return total;
}

int main(int argc, const char** argv)
{
	::Boxfish::Boxfish engine;
	Position position = CreateStartingPosition();
	engine.SetPosition(position);
	//engine.SetPosition(CreatePositionFromFEN("r3kb1r/ppp1pp2/n4n1p/1NqpQ1p1/6b1/BP1BP3/P1PP1PPP/R3K1NR b KQkq - 2 9"));

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
				PositionHistory& history = engine.GetSearch().GetHistory();
				history.Clear();
				history.Push(engine.GetCurrentPosition());
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
							history.Push(position);
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
			else
			{
				engine.GetSearch().GetHistory().Push(engine.GetCurrentPosition());
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

				Move move = search.Go(depth);
				std::cout << "bestmove " << FormatMove(move, false) << std::endl;
			}
			else
			{
				size_t timeString = args.find("time ");
				if (timeString != std::string_view::npos)
				{
					size_t space = args.find_first_of(' ', depthString + 5);
					int time = std::stoi(std::string(args.substr(timeString + 5, space - timeString - 5)));
					SearchLimits limits;
					limits.Milliseconds = time;
					Search& search = engine.GetSearch();
					search.SetLimits(limits);
					search.SetCurrentPosition(engine.GetCurrentPosition());

					Move move = search.Go(50);
					std::cout << "bestmove " << FormatMove(move, false) << std::endl;
				}
			}
		}
		else if (command == "eval")
		{
			EvaluationResult result = EvaluateDetailed(engine.GetCurrentPosition());
			std::cout << result.PieceSquares[TEAM_WHITE] << " " << result.PieceSquares[TEAM_BLACK] << std::endl;
			std::cout << result.GetTotal(engine.GetCurrentPosition().TeamToPlay) << " GameStage: " << result.GameStage << std::endl;
		}
		else if (command == "perft")
		{
			int depth = std::stoi(std::string(args));
			Position pos = engine.GetCurrentPosition();
			PerftRoot(pos, depth);
		}
		else if (command == "moves")
		{
			MoveGenerator generator(engine.GetCurrentPosition());
			MoveList moves = generator.GetPseudoLegalMoves();
			generator.FilterLegalMoves(moves);
			if (!moves.Empty())
			{
				for (int i = 0; i < moves.MoveCount; i++)
				{
					std::cout << FormatMove(moves.Moves[i]) << std::endl;
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
