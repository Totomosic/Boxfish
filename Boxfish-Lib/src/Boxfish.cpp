#include "Boxfish.h"

namespace Boxfish
{

	Boxfish::Boxfish()
		: m_CurrentPosition(), m_Search()
	{
		Logger::Init();
		Random::Init();
		InitZobristHash();
		InitRays();
		InitAttacks();
		InitEvaluation();
	}

	Search& Boxfish::GetSearch()
	{
		return m_Search;
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
