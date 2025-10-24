//
// Created by gabe on 6/24/25.
//
#include "components/Components.h"
#include "core/Entity.h"
#include "utils/Utils.h"
#include "imgui.h"
#include "ozz/animation/runtime/track.h"
#include "rendering/particles/ParticleManager.h"
#include "animation/AnimationManager.h"
#include "scripting/ScriptManager.h"
#include "glm/gtc/type_ptr.hpp"
#include "physics/PhysicsManager.h"
#include "TransformComponent.h"
#include "RigidBodyComponent.h"
#include "PlayerControllerComponent.h"
#include "rendering/ui/InspectorUI.h"

namespace Engine::Components {
	void Transform::OnRemoved(Entity& entity)
	{
	}
	void Transform::OnAdded(Entity& entity)
	{
	}

	void Transform::SyncWithPhysics(Entity& entity)
	{
		// Use world-space transform
		glm::vec3 worldPos = worldPosition;
		glm::quat worldRot = worldRotation;

		if (entity.HasComponent<RigidBodyComponent>()) {
			auto&          rb             = entity.GetComponent<RigidBodyComponent>();
			BodyInterface& body_interface = Engine::GetPhysics().GetPhysicsSystem()->GetBodyInterface();

			JPH::RVec3 joltPos(worldPos.x, worldPos.y, worldPos.z);
			JPH::Quat  joltRot(worldRot.x, worldRot.y, worldRot.z, worldRot.w);

			body_interface.SetPositionAndRotation(rb.bodyID, joltPos, joltRot, EActivation::Activate);
		}

		if (entity.HasComponent<PlayerControllerComponent>()) {
			auto& player = entity.GetComponent<PlayerControllerComponent>();
			player.SetPosition(worldPos);
			player.SetRotation(worldRot);
		}
	}


	void Transform::RenderInspector(Entity& entity)
	{
		bool updatePhysicsPositionManually = false;

		glm::vec3 displayPos = localPosition;
		if (LeftLabelDragFloat3("Position", glm::value_ptr(displayPos), 0.1f)) {
			localPosition                 = displayPos;
			updatePhysicsPositionManually = true;
		}

		glm::vec3 eulerAngles = glm::degrees(glm::eulerAngles(worldRotation));
		if (LeftLabelDragFloat3("Rotation", glm::value_ptr(eulerAngles), 1.0f)) {
			worldRotation                 = glm::quat(glm::radians(eulerAngles));
			updatePhysicsPositionManually = true;
		}

		if (updatePhysicsPositionManually) {
			SyncWithPhysics(entity);
		}

		LeftLabelDragFloat3("Scale", glm::value_ptr(localScale), 0.1f);
	}


	void Transform::AddBindings()
	{
		auto& lua = GetScriptManager().lua;

		// glm::vec3
		lua.new_usertype<glm::vec3>(
		    "vec3",
		    sol::constructors<glm::vec3(), glm::vec3(float, float, float)>(),
		    "x",
		    &glm::vec3::x,
		    "y",
		    &glm::vec3::y,
		    "z",
		    &glm::vec3::z,
		    "normalize",
		    [](const glm::vec3& v) {
			    if (glm::length2(v) == 0.0f) return v;
			    return glm::normalize(v);
		    },
		    "cross",
		    [](const glm::vec3& a, const glm::vec3& b) { return glm::cross(a, b); },
		    "length",
		    [](const glm::vec3& v) { return glm::length(v); },
		    "dot",
		    [](const glm::vec3& a, const glm::vec3& b) { return glm::dot(a, b); });

		lua.set_function("vec3", [](float x, float y, float z) { return glm::vec3(x, y, z); });

		lua.new_usertype<glm::vec2>("vec2", sol::constructors<glm::vec2(), glm::vec2(float, float)>(), "x", &glm::vec2::x, "y", &glm::vec2::y);
		lua.set_function("vec2", [](float x, float y) { return glm::vec2(x, y); });

		// glm::quat
		lua.new_usertype<glm::quat>("quat", sol::constructors<glm::quat(), glm::quat(float, float, float, float)>(), "w", &glm::quat::w, "x", &glm::quat::x, "y", &glm::quat::y, "z", &glm::quat::z);
		lua.set_function("quat", [](float w, float x, float y, float z) { return glm::quat(w, x, y, z); });

		// Transform (world-space access)
		lua.new_usertype<Transform>("Transform",

		                            // world-space accessors
		                            "position",
		                            &Transform::worldPosition,
		                            "rotation",
		                            &Transform::worldRotation,
		                            "scale",
		                            &Transform::worldScale,

		                            // utility methods
		                            "setRotation",
		                            &Transform::SetRotation,
		                            "GetEulerAngles",
		                            &Transform::GetEulerAngles);
	}

} // namespace Engine::Components