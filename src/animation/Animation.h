//
// Created by gabe on 9/13/25.
//

#pragma once


#include "ozz/animation/runtime/animation.h"


namespace Engine {
	// Engine asset wrapper around an ozz runtime animation.
	// Prefer this type in engine code; only pass Runtime() into ozz jobs.
	class Animation {
	  public:
		Animation() = default;
		~Animation();

		std::string name;
		// Owned ozz clip data (loaded from .anim / .ozz).
		ozz::animation::Animation* source = nullptr;

		[[nodiscard]] bool IsValid() const { return source != nullptr; }

		// ozz runtime object for SamplingJob / duration queries.
		[[nodiscard]] ozz::animation::Animation* Runtime() const { return source; }

		[[nodiscard]] float Duration() const
		{
			return source ? source->duration() : 0.f;
		}

		[[nodiscard]] int NumTracks() const
		{
			return source ? source->num_tracks() : 0;
		}
	};
} // namespace Engine