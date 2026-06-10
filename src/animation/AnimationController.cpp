//
// Created by Gabe on 2/28/2026.
//

#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/track.h"
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/containers/vector.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation/runtime/blending_job.h>

#include "AnimationController.h"

namespace Engine {

    void AnimationController::Init(ozz::animation::Skeleton* skeleton) {
        this->skeleton = skeleton;

        if (!skeleton)
            return;


        soa_joint_count = skeleton->num_soa_joints();

        // Reserve reasonable capacity to avoid reallocations
        //players.reserve(4);
        layers.reserve(4);
    }

    void AnimationController::Update(float dt) {
        for (auto& p : players)
        {
            Animation* anim =
                    GetAssetManager().Get(p->animation);

            if (!anim || !anim->source)
                continue;

            const float duration = anim->source->duration();

            // Advance normalized time
            p->time += (dt * p->playbackSpeed) / duration;

            if (p->looping)
            {
                p->time = fmod(p->time, 1.0f);
                if (p->time < 0.f)
                    p->time += 1.0f;
            }
            else
            {
                p->time = std::clamp(p->time, 0.f, 1.f);
            }
            const float blendSpeed = 6.0f; // tweakable

            p->weight +=
                    (p->targetWeight - p->weight)
                    * std::min(1.0f, dt * blendSpeed);
        }

        float totalWeight = 0.f;

        for (auto& p : players)
            totalWeight += p->weight;

        if (totalWeight > 0.0001f)
        {
            float inv = 1.0f / totalWeight;

            for (auto& p : players)
                p->weight *= inv;
        }
    }

    void AnimationController::Eval(ozz::span<ozz::math::SoaTransform> output) {
        std::vector<ozz::animation::BlendingJob::Layer> layers;

        for (auto& player : players) {
            Animation* anim = player->animation.IsValid()
                              ? GetAssetManager().Get(player->animation)
                              : nullptr;
            if (!anim || !anim->source) continue;

            const float ratio = anim->source->duration() > 0.f
                                ? player->time / anim->source->duration()
                                : 0.f;

            ozz::animation::SamplingJob job;
            job.animation = anim->source;
            job.context   = &player->context;
            job.ratio     = ratio;
            job.output    = ozz::make_span(player->localPose);

            if (!job.Run()) {
                GetDefaultLogger()->error("Failed to sample animation player");
                continue;
            }

            ozz::animation::BlendingJob::Layer layer;
            layer.transform = ozz::make_span(player->localPose);
            layer.weight    = player->weight;
            layers.push_back(layer);
        }

        if (layers.empty()) return;

        if (layers.size() == 1 && layers[0].weight >= 1.f) {
            ozz::span<const ozz::math::SoaTransform> src = layers[0].transform;
            std::copy(src.begin(), src.end(), output.begin());
            return;
        }

        ozz::animation::BlendingJob blend;
        blend.threshold = ozz::animation::BlendingJob().threshold;
        blend.layers    = ozz::make_span(layers);
        blend.rest_pose = skeleton->joint_rest_poses();
        blend.output    = output;

        if (!blend.Run()) {
            GetDefaultLogger()->error("Failed to blend animation layers");
        }
    }

    Engine::AnimationPlayer&
    AnimationController::Play(const AssetHandle<Animation>& anim)
    {
        ENGINE_VERIFY(skeleton != nullptr, "AnimationController::Play called without skeleton");

        // Try to reuse existing player
        for (auto& p : players)
        {
            if (p->animation == anim)
            {
                p->targetWeight = 1.0f;
                return *p;
            }
        }

        // Validate animation
        if (!anim.IsValid())
        {
            spdlog::error("AnimationController::Play: invalid animation");
            ENGINE_VERIFY(false, "Invalid animation");
        }

        Animation* animation = GetAssetManager().Get(anim);

        if (!animation || !animation->source)
        {
            spdlog::error("AnimationController::Play: invalid animation");
            ENGINE_VERIFY(false, "Invalid animation");
        }



        players.push_back(std::make_unique<AnimationPlayer>());
        AnimationPlayer& ref = *players.back();

        // Initialize
        ref.animation = anim;
        ref.time = 0.f;
        ref.playbackSpeed = 1.f;

        ref.weight = 0.f;
        ref.targetWeight = 1.f;
        ref.looping = true;

        ref.context.Resize(animation->source->num_tracks());
        ref.localPose.resize(soa_joint_count);

        // Fade out others
        for (auto& p : players)
        {
            if (p.get() != &ref)
                p->targetWeight = 0.f;
        }

        return ref;
    }
}