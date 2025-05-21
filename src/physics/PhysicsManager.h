#pragma once

#include <Jolt/Jolt.h>

// Jolt includes
#include "components/Components.h"

#include "entt/entt.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

#include "physics/PhysicsInterfaces.h"
#include "spdlog/spdlog.h"

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>


using namespace JPH;
using namespace JPH::literals;

namespace Engine {

	void      DecomposeMatrix(const JPH::RMat44& mat, glm::vec3& position, glm::quat& rotation, glm::vec3& scale);
	glm::mat4 CalculateModelMatrix(const Engine::Components::Transform& transform);


	const uint cMaxBodies             = 1024;
	const uint cNumBodyMutexes        = 0;
	const uint cMaxBodyPairs          = 1024;
	const uint cMaxContactConstraints = 1024;


	class PhysicsManager {
	  public:
		static void TraceImpl(const char* inFMT, ...);
		static bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, uint inLine);


		static void                              Initialize();
		static void                              CleanUp(entt::registry& registry);
		static void                              Update(float dt);
		static void                              SyncPhysicsEntities(entt::registry& registry);
		static std::shared_ptr<PhysicsSystem>    GetPhysicsSystem();
		static BPLayerInterfaceImpl              broad_phase_layer_interface;
		static ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;
		static ObjectLayerPairFilterImpl         object_vs_object_layer_filter;
		static ContactListenerImpl               contact_listener;
		static BodyActivationListenerImpl        body_activation_listener;
	};
} // namespace Engine