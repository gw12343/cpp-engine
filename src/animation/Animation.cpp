//
// Created by gabe on 9/14/25.
//

#include "Animation.h"

#include "ozz/animation/runtime/animation.h"

namespace Engine {
	Engine::Animation::~Animation()
	{
		delete source;
		source = nullptr;
	}

	float Animation::Duration() const
	{
		return source ? source->duration() : 0.f;
	}

	int Animation::NumTracks() const
	{
		return source ? source->num_tracks() : 0;
	}

} // namespace Engine