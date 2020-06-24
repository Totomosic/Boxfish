#pragma once
#include "Logging.h"
#include "Position.h"
#include "PositionUtils.h"
#include "Utils.h"
#include "Rays.h"

#include "Attacks.h"
#include "MoveGenerator.h"
#include "Evaluation.h"

namespace Boxfish
{

	class BOX_API Boxfish
	{
	private:
		Position m_CurrentPosition;

	public:
		Boxfish();

		const Position& GetCurrentPosition() const;
		void SetPosition(const Position& position);
	};

}
