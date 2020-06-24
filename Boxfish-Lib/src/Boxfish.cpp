#include "Boxfish.h"

namespace Boxfish
{

	Boxfish::Boxfish()
		: m_CurrentPosition()
	{
		Logger::Init();
		InitRays();
		InitAttacks();
		InitEvaluation();
	}

	const Position& Boxfish::GetCurrentPosition() const
	{
		return m_CurrentPosition;
	}

	void Boxfish::SetPosition(const Position& position)
	{
		m_CurrentPosition = position;
	}

}
