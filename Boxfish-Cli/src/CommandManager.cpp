#include "CommandManager.h"

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

	CommandManager::CommandManager()
		: m_CommandMap(), m_CurrentPosition(CreateStartingPosition()), m_Search(128 * 1024 * 1024), m_Settings(), m_Searching(false), m_SearchThread()
	{
		m_CommandMap["help"] = [this](const std::vector<std::string>& args)
		{
			Help();
		};

		m_CommandMap["isready"] = [this](const std::vector<std::string>& args)
		{
			IsReady();
		};

		m_CommandMap["ucinewgame"] = [this](const std::vector<std::string>& args)
		{
			NewGame();
		};

		m_CommandMap["setoption"] = [this](const std::vector<std::string>& args)
		{
			if (args.size() > 1)
			{
				auto it = args.begin();
				while (it != args.end() && *it != "name")
					it++;
				if (it != args.end())
					it++;
				if (it != args.end())
				{
					std::string name = *it;
					it++;
					while (it != args.end() && *it != "value")
					{
						name += " " + *it;
						it++;
					}
					if (it != args.end())
						it++;
					if (it != args.end())
					{
						std::string value = *it;
						while (it != args.end())
						{
							value += " " + *it;
							it++;
						}
						SetOption(name, value);
					}
				}
			}
		};

		m_CommandMap["d"] = [this](const std::vector<std::string>& args)
		{
			PrintBoard();
		};

		m_CommandMap["position"] = [this](const std::vector<std::string>& args)
		{
			if (args.size() > 0)
			{
				auto it = args.begin();
				if (*it == "fen")
				{
					it++;
					std::string fen = *it++;
					while (it != args.end() && *it != "moves")
					{
						fen += " " + *it++;
					}
					SetPositionFen(fen);
				}
				if (it != args.end() && *it == "startpos")
				{
					SetPositionFen(GetFENFromPosition(CreateStartingPosition()));
					it++;
				}
				if (it != args.end() && *it == "moves")
				{
					ApplyMoves({ it + 1, args.end() });
				}
			}
		};

		m_CommandMap["eval"] = [this](const std::vector<std::string>& args)
		{
			Eval();
		};

		m_CommandMap["perft"] = [this](const std::vector<std::string>& args)
		{
			if (args.size() > 0)
			{
				int depth = std::stoi(args[0]);
				Perft(depth);
			}
		};

		m_CommandMap["go"] = [this](const std::vector<std::string>& args)
		{
			if (args.size() > 0)
			{
				if (args.size() > 1)
				{
					if (args[0] == "depth")
					{
						int depth = std::stoi(args[1]);
						GoDepth(depth);
					}
					else if (args[0] == "movetime")
					{
						int milliseconds = std::stoi(args[1]);
						GoTime(milliseconds);
					}
				}
				else if (args[0] == "ponder")
				{
					GoPonder();
				}
			}
		};

		m_CommandMap["moves"] = [this](const std::vector<std::string>& args)
		{
			Moves();
		};

		m_CommandMap["stop"] = [this](const std::vector<std::string>& args)
		{
			Stop();
		};

		m_CommandMap["quit"] = [this](const std::vector<std::string>& args)
		{
			Quit();
		};

		ExecuteCommand("ucinewgame");
	}

	CommandManager::~CommandManager()
	{
		if (m_SearchThread.joinable())
			m_SearchThread.join();
	}

	void CommandManager::ExecuteCommand(const std::string& uciCommand)
	{
		std::vector<std::string> commandParts = Split(uciCommand, " ");
		if (commandParts.size() > 0)
		{
			std::string& command = commandParts[0];
			if (m_CommandMap.find(command) != m_CommandMap.end())
			{
				std::vector<std::string> args = { commandParts.begin() + 1, commandParts.end() };
				m_CommandMap.at(command)(args);
			}
			else
			{
				std::cout << "Unknown command: " << command << std::endl;
			}
		}
	}

	void CommandManager::Help()
	{
		std::cout << "Available Commands:" << std::endl;
		std::cout << "* isready" << std::endl;
		std::cout << "\tCheck if the engine is ready." << std::endl;
		std::cout << "* ucinewgame" << std::endl;
		std::cout << "\tInform the engine that this is a new game." << std::endl;
		std::cout << "* d" << std::endl;
		std::cout << "\tPrint the current position." << std::endl;
		std::cout << "* position [fen <fenstring> | startpos] [moves <moves>...]" << std::endl;
		std::cout << "\tSet the current position from a FEN string or to the starting position." << std::endl;
		std::cout << "\tAdditionally, apply a list of moves to the given position." << std::endl;
		std::cout << "\tIf fen or startpos is not provided the moves will be applied to the current position." << std::endl;
		std::cout << "* eval" << std::endl;
		std::cout << "\tPrint the static evaluation for the current position." << std::endl;
		std::cout << "* perft <depth>" << std::endl;
		std::cout << "\tPerformance test move generation for a given depth in the current position." << std::endl;
		std::cout << "* go" << std::endl;
		std::cout << "\tMain command to begin searching in the current position." << std::endl;
		std::cout << "\tAccepts a number of different arguments:" << std::endl;
		std::cout << "\t* ponder" << std::endl;
		std::cout << "\t\tStart searching in ponder mode, searching infinitely until a stop command." << std::endl;
		std::cout << "\t* depth <depth>" << std::endl;
		std::cout << "\t\tSearch the current position to a given depth." << std::endl;
		std::cout << "\t* movetime <time_ms>" << std::endl;
		std::cout << "\t\tSearch the current position for a given number of milliseconds." << std::endl;
		std::cout << "* stop" << std::endl;
		std::cout << "\tStop searching as soon as possible." << std::endl;
		std::cout << "* quit" << std::endl;
		std::cout << "\tQuit the program as soon as possible." << std::endl;
	}

	void CommandManager::IsReady()
	{
		std::cout << "readyok" << std::endl;
	}

	void CommandManager::NewGame()
	{
		if (!m_Searching)
		{
			m_Search.Reset();
			m_CurrentPosition = CreateStartingPosition();
		}
	}

	void CommandManager::PrintBoard()
	{
		std::cout << m_CurrentPosition << std::endl;
		std::cout << "FEN: " << GetFENFromPosition(m_CurrentPosition) << std::endl;
		std::cout << "Hash: " << std::hex << m_CurrentPosition.Hash.Hash << std::dec << std::endl;
	}

	void CommandManager::SetOption(std::string name, const std::string& value)
	{
		std::transform(name.begin(), name.end(), name.begin(), [](char c) { return std::tolower(c); });
		if (name == "multipv")
		{
			m_Settings.MultiPV = std::max(0, std::stoi(value));
		}
		if (name == "skill level")
		{
			m_Settings.SkillLevel = std::min(std::max(0, std::stoi(value)), 20);
		}
		m_Search.SetSettings(m_Settings);
	}

	void CommandManager::SetPositionFen(const std::string& fen)
	{
		if (!m_Searching)
		{
			m_Search.Reset();
			m_CurrentPosition = CreatePositionFromFEN(fen);
		}
	}

	void CommandManager::ApplyMoves(const std::vector<std::string>& moves)
	{
		Move moveBuffer[MAX_MOVES];
		if (!m_Searching)
		{
			for (const std::string& moveString : moves)
			{
				MoveList moves(moveBuffer);
				MoveGenerator generator(m_CurrentPosition);
				generator.GetPseudoLegalMoves(moves);
				generator.FilterLegalMoves(moves);
				Move move = CreateMoveFromString(m_CurrentPosition, moveString);

				bool legal = false;
				for (int i = 0; i < moves.MoveCount; i++)
				{
					if (move == moves.Moves[i])
					{
						ApplyMove(m_CurrentPosition, move);
						m_Search.PushPosition(m_CurrentPosition);
						legal = true;
						break;
					}
				}
				if (!legal)
				{
					std::cout << "Move " << moveString << " is not valid." << std::endl;
					break;
				}
			}
		}
	}

	void CommandManager::Eval()
	{
		EvaluationResult evaluation = EvaluateDetailed(m_CurrentPosition);
		std::cout << FormatEvaluation(evaluation) << std::endl;
	}

	void CommandManager::Perft(int depth)
	{
		if (!m_Searching)
		{
			m_Search.Perft(m_CurrentPosition, depth);
		}
	}

	void CommandManager::GoDepth(int depth)
	{
		if (!m_Searching)
		{
			m_Searching = true;
			if (m_SearchThread.joinable())
				m_SearchThread.join();
			m_SearchThread = std::thread([this, depth]()
			{
				SearchLimits limits;
				limits.Depth = depth;
				Move bestMove = m_Search.SearchBestMove(m_CurrentPosition, limits);
				std::cout << "bestmove " << UCI::FormatMove(bestMove) << std::endl;
				m_Searching = false;
			});
		}
	}

	void CommandManager::GoTime(int milliseconds)
	{
		if (!m_Searching)
		{
			m_Searching = true;
			if (m_SearchThread.joinable())
				m_SearchThread.join();
			m_SearchThread = std::thread([this, milliseconds]()
			{
				SearchLimits limits;
				limits.Milliseconds = milliseconds;
				Move bestMove = m_Search.SearchBestMove(m_CurrentPosition, limits);
				std::cout << "bestmove " << UCI::FormatMove(bestMove) << std::endl;
				m_Searching = false;
			});
		}
	}

	void CommandManager::GoPonder()
	{
		if (!m_Searching)
		{
			m_Searching = true;
			if (m_SearchThread.joinable())
				m_SearchThread.join();
			m_SearchThread = std::thread([this]()
			{
				m_Search.Ponder(m_CurrentPosition);
				m_Searching = false;
			});
		}
	}

	void CommandManager::Moves()
	{
		Move moveBuffer[MAX_MOVES];
		MoveGenerator generator(m_CurrentPosition);
		MoveList moves(moveBuffer);
		generator.GetPseudoLegalMoves(moves);
		generator.FilterLegalMoves(moves);
		for (int i = 0; i < moves.MoveCount; i++)
		{
			std::cout << UCI::FormatMove(moves.Moves[i]) << std::endl;
		}
	}

	void CommandManager::Stop()
	{
		if (m_Searching)
		{
			m_Search.Stop();
			m_SearchThread.join();
		}
	}

	void CommandManager::Quit()
	{
		Stop();
		exit(0);
	}

}
