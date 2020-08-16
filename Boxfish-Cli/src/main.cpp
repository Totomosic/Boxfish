#include "Boxfish.h"
#include "CommandManager.h"
#include <fstream>

using namespace Boxfish;

int main(int argc, const char** argv)
{
	Init();
	std::string version = __TIMESTAMP__;

	std::cout << "Boxfish " << version << " by J. Morrison" << std::endl;

	CommandManager commands;
 
	char buffer[4096];
	while (true)
	{
		std::cin.getline(buffer, sizeof(buffer));
		commands.ExecuteCommand(buffer);
	}
	return 0;
}
