//
// Created by Gabe on 2/28/2026.
//

#include "AnimationPlayer.h"

#include <ozz/animation/runtime/blending_job.h>

#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/memory.hpp>


#pragma once
namespace Engine {
    class AnimationController {
    public:
        void Init(ozz::animation::Skeleton* skeleton);
        void Update(float dt);
        void Eval(ozz::span<ozz::math::SoaTransform> output);

        Engine::AnimationPlayer& Play(const AssetHandle<Animation>& anim);

        AnimationController() = default;

        AnimationController(const AnimationController&)            = delete;
        AnimationController& operator=(const AnimationController&) = delete;
        AnimationController(AnimationController&&)                 = default;
        AnimationController& operator=(AnimationController&&)      = default;

        template <class Archive>
        void serialize(Archive& ar)
        {
            ar(CEREAL_NVP(players));
        }

        std::vector<std::unique_ptr<AnimationPlayer>> players;

        float fadeDuration = 0.2f;
    private:
        int soa_joint_count;
        std::vector<ozz::animation::BlendingJob::Layer> layers;

        ozz::animation::Skeleton* skeleton = nullptr;
    };
}