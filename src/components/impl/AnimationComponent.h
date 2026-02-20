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

namespace Engine::Components {
	class AnimationComponent : public Component {
	  public:
		AssetHandle<Animation>                animation;
		ozz::animation::SamplingJob::Context* context   = nullptr;
		float                                 timescale = 0.0;

        std::vector<ozz::math::SoaTransform>* local_pose = nullptr;
        std::vector<ozz::math::Float4x4>*     model_pose = nullptr;

        ozz::animation::Skeleton* skeleton = nullptr;
        std::string               skeletonPath;

		template <class Archive>
		void serialize(Archive& ar)
		{
			ar(CEREAL_NVP(animation));
            ar(CEREAL_NVP(skeletonPath));
		}

		AnimationComponent() = default;
		void OnAdded(Entity& entity) override;
		void OnRemoved(Entity& entity) override;
		void RenderInspector(Entity& entity) override;

		void SetSkeleton(const std::string&  path);
		void SetAnimation(const AssetHandle<Animation>& animation);

		static void CleanAnimationContexts();

		static std::unordered_set<ozz::animation::SamplingJob::Context*> s_contexts;
	};
} // namespace Engine::Components

#endif // CPP_ENGINE_ANIMATIONCOMPONENT_H
