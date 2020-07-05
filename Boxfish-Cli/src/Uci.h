#pragma once
#include "Boxfish.h"

namespace Boxfish
{

	class UCI
	{
	private:
		::Boxfish::Boxfish m_Engine;

	public:
		UCI();

		void Start();
	};

}