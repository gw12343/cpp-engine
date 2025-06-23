//
// Created by gabe on 6/22/25.
//

#include "EngineData.h"

namespace Engine {
	EngineData& Get()
	{
		static EngineData instance;
		return instance;
	}

} // namespace Engine