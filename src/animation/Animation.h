//
// Created by gabe on 9/13/25.
//

#pragma once

#include <string>
#include "ozz/animation/runtime/animation.h"
#include <memory>

namespace Engine {
	class Animation {
	  public:
		Animation() = default;
		~Animation();
		std::string                name;
		ozz::animation::Animation* source;
	};
} // namespace Engine