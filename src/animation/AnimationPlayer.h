//
// Created by Gabe on 2/28/2026.
//

#pragma once

#include "ozz/animation/runtime/animation.h"
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/containers/vector.h>
#include <cereal/cereal.hpp>

namespace Engine {
    struct AnimationPlayer {
        AnimationPlayer() = default;

        AssetHandle<Animation> animation;

        ozz::animation::SamplingJob::Context context;

        float time = 0.f;
        float weight = 1.f;
        float targetWeight = 1.f;
        float playbackSpeed = 1.f;
        bool looping = true;

        ozz::vector<ozz::math::SoaTransform> localPose;


        template <class Archive>
        void serialize(Archive& ar)
        {
            ar(CEREAL_NVP(animation));
            ar(CEREAL_NVP(time));
            ar(CEREAL_NVP(weight));
            ar(CEREAL_NVP(targetWeight));
            ar(CEREAL_NVP(playbackSpeed));
            ar(CEREAL_NVP(looping));
            // context and localPose are runtime generated
        }
    };
};