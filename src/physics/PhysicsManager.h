#pragma once

#ifdef AddJob
#undef AddJob
#endif
#include <Jolt/Jolt.h>

// Jolt includes
#include "components/Components.h"

#include "entt/entt.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

#include "physics/PhysicsInterfaces.h"
#include "spdlog/spdlog.h"
#include "core/module/Module.h"

#include <Jolt/Core/Factory.h>

#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/TriangleShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>
#include <Jolt/RegisterTypes.h>
#include "core/Entity.h"
#include "components/impl/TransformComponent.h"
#include "Jolt/Physics/Character/CharacterVirtual.h"

using namespace JPH;
using namespace JPH::literals;

namespace Engine {

	void      DecomposeMatrix(const JPH::RMat44& mat, glm::vec3& position, glm::quat& rotation, glm::vec3& scale);
	glm::mat4 CalculateModelMatrix(const Engine::Components::Transform& transform);


	const uint cMaxBodies             = 1024;
	const uint cNumBodyMutexes        = 0;
	const uint cMaxBodyPairs          = 1024;
	const uint cMaxContactConstraints = 1024;


	class PhysicsManager : public Module {
	  public:
		// bool        isPhysicsPaused = true;
		static void TraceImpl(const char* inFMT, ...);
		static bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, uint inLine);


		void        onInit() override;
		void        onUpdate(float dt) override;
		void        onGameStart() override {}
		void        onShutdown() override;
		std::string name() const override { return "PhysicsManger"; };
		void        setLuaBindings() override;

		void                              SyncPhysicsEntities();
		void                              SyncCharacterEntities();
		std::shared_ptr<PhysicsSystem>    GetPhysicsSystem();
		std::shared_ptr<CharacterVirtual> GetCharacter();

		BPLayerInterfaceImpl              broad_phase_layer_interface;
		ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;
		ObjectLayerPairFilterImpl         object_vs_object_layer_filter;
		ContactListenerImpl               contact_listener;
		BodyActivationListenerImpl        body_activation_listener;

		std::unordered_map<JPH::BodyID, Entity> bodyToEntityMap;
	};
} // namespace Engine