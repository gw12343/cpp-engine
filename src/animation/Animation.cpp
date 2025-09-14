//
// Created by gabe on 9/14/25.
//

#include "Animation.h"

#include <iostream>
namespace Engine {
	Engine::Animation::~Animation()
	{
		delete source;
	}

} // namespace Engine