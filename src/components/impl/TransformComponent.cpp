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
		if (entity.HasComponent<RigidBodyComponent>()) {
			auto&          rb             = entity.GetComponent<RigidBodyComponent>();
			BodyInterface& body_interface = Engine::GetPhysics().GetPhysicsSystem()->GetBodyInterface();
			// convert pos and rot to jolt types
			RVec3 joltPos = RVec3(position.x, position.y, position.z);
			Quat  joltRot = Quat(rotation.x, rotation.y, rotation.z, rotation.w);
			body_interface.SetPositionAndRotation(rb.bodyID, joltPos, joltRot, EActivation::Activate);
		}

		if (entity.HasComponent<PlayerControllerComponent>()) {
			auto& player = entity.GetComponent<PlayerControllerComponent>();

			player.SetPosition(position);
			player.SetRotation(rotation);
		}
	}

	void Transform::RenderInspector(Entity& entity)
	{
		bool updatePhysicsPositionManually = false;
		if (LeftLabelDragFloat3("Position", glm::value_ptr(position), 0.1f)) {
			updatePhysicsPositionManually = true;
		}


		glm::vec3 eulerAngles = GetEulerAngles();
		if (LeftLabelDragFloat3("Rotation", glm::value_ptr(eulerAngles), 1.0f)) {
			SetRotation(eulerAngles);
			updatePhysicsPositionManually = true;
		}

		if (updatePhysicsPositionManually) {
			SyncWithPhysics(entity);
		}

		LeftLabelDragFloat3("Scale", glm::value_ptr(scale), 0.1f);
	}

	void Transform::AddBindings()
	{
		auto& lua = GetScriptManager().lua;


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
			    if (glm::length2(v) == 0.0f) return v; // avoid NaN
			    return glm::normalize(v);
		    },

		    // cross product
		    "cross",
		    [](const glm::vec3& a, const glm::vec3& b) { return glm::cross(a, b); },

		    // vector length
		    "length",
		    [](const glm::vec3& v) { return glm::length(v); },

		    // dot product
		    "dot",
		    [](const glm::vec3& a, const glm::vec3& b) { return glm::dot(a, b); });


		lua.set_function("vec3", [](float x, float y, float z) { return glm::vec3(x, y, z); });

		lua.new_usertype<glm::vec2>("vec2", sol::constructors<glm::vec2(), glm::vec2(float, float)>(), "x", &glm::vec2::x, "y", &glm::vec2::y);
		lua.set_function("vec2", [](float x, float y, float z) { return glm::vec2(x, y); });


		lua.new_usertype<glm::quat>("quat",
		                            sol::constructors<glm::quat(),                          // identity
		                                              glm::quat(float, float, float, float) // (w, x, y, z)
		                                              >(),
		                            "w",
		                            &glm::quat::w,
		                            "x",
		                            &glm::quat::x,
		                            "y",
		                            &glm::quat::y,
		                            "z",
		                            &glm::quat::z);

		// Convenience factory function for Lua
		lua.set_function("quat", [](float w, float x, float y, float z) { return glm::quat(w, x, y, z); });


		lua.new_usertype<Transform>("Transform",
		                            "position",
		                            &Transform::position,
		                            "rotation",
		                            &Transform::rotation,
		                            "scale",
		                            &Transform::scale,

		                            "setRotation",
		                            &Transform::SetRotation,
		                            "GetEulerAngles",
		                            &Transform::GetEulerAngles);
	}
} // namespace Engine::Components