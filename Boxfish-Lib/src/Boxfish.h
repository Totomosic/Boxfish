#pragma once
#include "Logging.h"
#include "Position.h"
#include "PositionUtils.h"
#include "Utils.h"
#include "Rays.h"
#include "ZobristHash.h"
#include "TranspositionTable.h"
#include "Random.h"

#include "Attacks.h"
#include "MoveGenerator.h"
#include "Evaluation.h"

#include "Search.h"

namespace Boxfish
{

	class BOX_API Boxfish
	{
	private:
		Position m_CurrentPosition;
		Search m_Search;

	public:
		Boxfish();

		Search& GetSearch();
		const Position& GetCurrentPosition() const;
		void SetPosition(const Position& position);
	};

}
