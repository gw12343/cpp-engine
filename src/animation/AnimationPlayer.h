//
// Single-clip playback slot (ozz sampling state).
// AnimationComponent uses at most two of these: current + from (crossfade).
//

#pragma once

#include "ozz/animation/runtime/animation.h"
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/containers/vector.h>
#include <cereal/cereal.hpp>

#include "assets/AssetHandle.h"

namespace Engine {

	// Runtime sampling state for one clip. Never deleted mid-frame; slots are
	// fixed on the component and only activated/deactivated.
	struct AnimationTrack {
		AnimationHandle clip{};

		float time          = 0.f;
		float speed         = 1.f;
		bool  looping       = true;
		bool  active        = false;

		// Runtime only (not serialized)
		ozz::animation::SamplingJob::Context   context;
		ozz::vector<ozz::math::SoaTransform>   localPose;

		void Deactivate()
		{
			active  = false;
			clip    = {};
			time    = 0.f;
			// Keep context/localPose capacity for reuse
		}

		template <class Archive>
		void serialize(Archive& ar)
		{
			ar(CEREAL_NVP(clip), CEREAL_NVP(time), CEREAL_NVP(speed), CEREAL_NVP(looping), CEREAL_NVP(active));
		}
	};

} // namespace Engine
