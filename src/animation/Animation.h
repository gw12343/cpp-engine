//
// Created by gabe on 9/13/25.
//

#pragma once


#include "ozz/animation/runtime/animation.h"


namespace Engine {
	class Animation {
	  public:
		Animation() = default;
		~Animation();
		std::string                name;
		ozz::animation::Animation* source = nullptr;
	};
} // namespace Engine