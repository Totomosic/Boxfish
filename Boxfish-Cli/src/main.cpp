#include "Boxfish.h"

int main(int argc, const char** argv)
{
	Boxfish::Boxfish engine;
	Boxfish::Position position = Boxfish::CreateStartingPosition();
	engine.SetPosition(position);
	BOX_INFO(Boxfish::GetFENFromPosition(engine.GetCurrentPosition()));
	return 0;
}
