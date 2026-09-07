//
// Created by gabe on 9/13/25.
//

#pragma once

#include <string>

namespace ozz {
	namespace animation {
		class Animation;
	}
} // namespace ozz

namespace Engine {
	class Animation {
	  public:
		Animation() = default;
		~Animation();

		std::string name;
		ozz::animation::Animation* source = nullptr;

		[[nodiscard]] bool IsValid() const { return source != nullptr; }

		[[nodiscard]] ozz::animation::Animation* Runtime() const { return source; }

		[[nodiscard]] float Duration() const;
		[[nodiscard]] int   NumTracks() const;
	};
} // namespace Engine
