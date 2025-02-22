#pragma once
#include "Types.h"

namespace DirectUI
{
	class UILIB_API AutoVariant
	{
	public:
		AutoVariant();
		AutoVariant& operator=(const AutoVariant &);
		~AutoVariant();
	};
}