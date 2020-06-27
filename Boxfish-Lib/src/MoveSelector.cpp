#include "MoveSelector.h"

namespace Boxfish
{

	MoveSelector::MoveSelector(std::vector<Move>& legalMoves)
		: m_LegalMoves(legalMoves), m_RandEngine()
	{
		std::random_device rd;
		std::mt19937::result_type seed = rd() ^ (
			(std::mt19937::result_type)
			std::chrono::duration_cast<std::chrono::seconds>(
				std::chrono::system_clock::now().time_since_epoch()
				).count() +
			(std::mt19937::result_type)
			std::chrono::duration_cast<std::chrono::microseconds>(
				std::chrono::high_resolution_clock::now().time_since_epoch()
				).count());
		m_RandEngine.seed(seed);
	}

	bool MoveSelector::Empty() const
	{
		return m_LegalMoves.empty();
	}

	Move MoveSelector::GetNextMove()
	{
		BOX_ASSERT(!Empty(), "Move list is empty");
		std::uniform_int_distribution<uint32_t> dist(0, m_LegalMoves.size() - 1);
		auto it = m_LegalMoves.begin() + dist(m_RandEngine);
		Move mv = *it;
		m_LegalMoves.erase(it);
		return mv;
	}

}
