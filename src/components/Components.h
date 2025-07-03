#pragma once

#include <memory>
#include <string>
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
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/containers/vector.h>
#include <utility>
#include <sol/environment.hpp>
#include <sol/function.hpp>

using namespace JPH;
using namespace JPH::literals;

namespace ozz::animation {
	class Skeleton;
	class Animation;
	class LocalToModelJob;
} // namespace ozz::animation

namespace ozz::math {
	struct SoaTransform;
	struct Float4x4;
} // namespace ozz::math


namespace Engine {
	class Mesh;
	class Entity;

	// Component structs for the ECS system
	namespace Components {

		void RegisterAllComponentBindings();

		// Base Component class
		class Component {
		  public:
			Component()          = default;
			virtual ~Component() = default;

			virtual void OnAdded(Entity& entity) = 0;

			// New method for rendering component in inspector
			virtual void RenderInspector(Entity& entity) {}


			static void AddBindings() { SPDLOG_INFO("unimpl!"); }
		};

	} // namespace Components
} // namespace Engine