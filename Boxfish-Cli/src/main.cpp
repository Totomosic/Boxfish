#include "Boxfish.h"

int main(int argc, const char** argv)
{
	Boxfish::Boxfish engine;
	Boxfish::Position position = Boxfish::CreateStartingPosition();
	engine.SetPosition(position);
	BOX_INFO(Boxfish::GetFENFromPosition(engine.GetCurrentPosition()));
	std::cout << position << std::endl;
	return 0;
}
