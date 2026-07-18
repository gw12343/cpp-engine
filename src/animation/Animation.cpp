//
// Created by gabe on 9/14/25.
//

#include "Animation.h"


namespace Engine {
	Engine::Animation::~Animation()
	{
		delete source;
		source = nullptr;
	}

} // namespace Engine