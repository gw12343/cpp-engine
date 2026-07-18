//
// Created by gabe on 6/29/25.
//

#ifndef CPP_ENGINE_ANIMATIONCOMPONENT_H
#define CPP_ENGINE_ANIMATIONCOMPONENT_H



#define GLM_ENABLE_EXPERIMENTAL
#include "Jolt/Jolt.h"
#include "Jolt/Physics/Body/BodyActivationListener.h"
#include "Jolt/Physics/Body/BodyManager.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "rendering/Model.h"
#include "rendering/Shader.h"
#include "sound/SoundManager.h"
#include "spdlog/spdlog.h"
#include "Jolt/Physics/Collision/Shape/BoxShape.h"
#include "Jolt/Physics/Collision/Shape/SphereShape.h"
#include "Jolt/Physics/Collision/Shape/CapsuleShape.h"
#include "Jolt/Physics/Collision/Shape/CylinderShape.h"
#include "Jolt/Physics/Collision/Shape/TriangleShape.h"

#include <Effekseer.h>


#include <glm/gtx/quaternion.hpp>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/containers/vector.h>
#include <utility>
#include <sol/environment.hpp>
#include <sol/function.hpp>

#include "components/Components.h"
#include "animation/Animation.h"

#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/array.hpp>

#include <animation/AnimationPlayer.h>
#include <animation/AnimationController.h>

namespace Engine::Components {
    class AnimationComponent : public Component {
    public:
        // Controller owns the set of AnimationPlayers and blending logic
        AnimationController controller;

        // Blended output poses (written to by the system each frame)
        std::vector<ozz::math::SoaTransform>* local_pose = nullptr;
        std::vector<ozz::math::Float4x4>*     model_pose = nullptr;

        ozz::animation::Skeleton* skeleton     = nullptr;
        std::string               skeletonPath;

        template <class Archive>
        void serialize(Archive& ar)
        {
            ar(CEREAL_NVP(skeletonPath));
            ar(CEREAL_NVP(controller));
        }

        AnimationComponent() = default;

        AnimationComponent(const AnimationComponent& other)
        {
            skeletonPath = other.skeletonPath;
            // Deep copy only the serializable player data — runtime state is rebuilt on OnAdded
            for (const auto& player : other.controller.players) {
                auto& p = controller.players.emplace_back(std::make_unique<AnimationPlayer>());
                p->animation     = player->animation;
                p->looping       = player->looping;
                p->playbackSpeed = player->playbackSpeed;
                p->targetWeight  = player->targetWeight;
                // time, weight, context, localPose are runtime — leave at defaults
            }
        }

        AnimationComponent& operator=(const AnimationComponent& other)
        {
            if (this != &other) {
                skeletonPath = other.skeletonPath;
                controller.players.clear();
                for (const auto& player : other.controller.players) {
                    auto& p = controller.players.emplace_back(std::make_unique<AnimationPlayer>());
                    p->animation     = player->animation;
                    p->looping       = player->looping;
                    p->playbackSpeed = player->playbackSpeed;
                    p->targetWeight  = player->targetWeight;
                }
            }
            return *this;
        }

        AnimationComponent(AnimationComponent&&)                 = default;
        AnimationComponent& operator=(AnimationComponent&&)      = default;


        void OnAdded(Entity& entity) override;
        void OnRemoved(Entity& entity) override;
        void RenderInspector(Entity& entity) override;

        void SetSkeleton(const std::string& path);

        // Convenience helpers that delegate to the controller
        AnimationPlayer& PlayAnimation(const AssetHandle<Animation>& animation, bool loop = true);
        void CrossfadeTo(const AssetHandle<Animation>& animation, float fadeDuration = 0.2f);
        AnimationPlayer* GetActivePlayer();

        static void CleanAnimationContexts();
    };
} // namespace Engine::Components

#endif // CPP_ENGINE_ANIMATIONCOMPONENT_H

//
//template<>
//struct entt::component_traits<Engine::Components::AnimationComponent> {
//    static constexpr auto in_place_delete = true;
//    static constexpr auto page_size = ENTT_PACKED_PAGE;
//};