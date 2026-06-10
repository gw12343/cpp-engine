#pragma once



#define GLM_ENABLE_EXPERIMENTAL

#ifdef AddJob
#undef AddJob
#endif
#include "Jolt/Jolt.h"
#include "spdlog/spdlog.h"
#include "core/EngineData.h"

#include <Effekseer.h>

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
	class Entity;

	// Component structs for the ECS system
	namespace Components {

		void RegisterAllComponentBindings();

		// Base Component class
		class Component {
		  public:
			Component()          = default;
			virtual ~Component() = default;

			virtual void OnAdded(Entity& entity)   = 0;
			virtual void OnRemoved(Entity& entity) = 0;

			// New method for rendering component in inspector
			virtual void RenderInspector(Entity& entity) {}


			static void AddBindings() { GetDefaultLogger()->warn("No bindings specified for a module!"); }
		};

	} // namespace Components
} // namespace Engine