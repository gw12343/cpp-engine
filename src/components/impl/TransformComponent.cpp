//
// Created by gabe on 6/24/25.
//
#include "components/Components.h"
#include "core/Engine.h"
#include "core/Entity.h"
#include "utils/Utils.h"

#include "imgui.h"
#include "ozz/animation/runtime/track.h"
#include "rendering/particles/ParticleManager.h"
#include "animation/AnimationManager.h"
#include "scripting/ScriptManager.h"
#include "glm/gtc/type_ptr.hpp"

namespace Engine::Components {
	void Transform::OnAdded(Entity& entity)
	{
	}

	void Transform::RenderInspector(Entity& entity)
	{
		bool updatePhysicsPositionManually = false;
		if (ImGui::DragFloat3("Position", glm::value_ptr(position), 0.1f)) {
			updatePhysicsPositionManually = true;
		}

		glm::vec3 eulerAngles = GetEulerAngles();
		if (ImGui::DragFloat3("Rotation", glm::value_ptr(eulerAngles), 1.0f)) {
			SetRotation(eulerAngles);
			updatePhysicsPositionManually = true;
		}

		if (updatePhysicsPositionManually) {
			if (entity.HasComponent<RigidBodyComponent>()) {
				auto& rb = entity.GetComponent<RigidBodyComponent>();
				ENGINE_VERIFY(rb.physicsSystem, "Transform::RenderInspector: RigidBodyComponent has null physicsSystem");
				BodyInterface& body_interface = rb.physicsSystem->GetBodyInterface();
				// convert pos and rot to jolt types
				RVec3 joltPos = RVec3(position.x, position.y, position.z);
				Quat  joltRot = Quat::sEulerAngles(RVec3(glm::radians(eulerAngles.x), glm::radians(eulerAngles.y), glm::radians(eulerAngles.z)));
				body_interface.SetPositionAndRotation(rb.bodyID, joltPos, joltRot, EActivation::Activate);
			}
		}

		ImGui::DragFloat3("Scale", glm::value_ptr(scale), 0.1f);
	}

	void Transform::AddBindings()
	{
		auto& lua = GetScriptManager().lua;
		

		lua.new_usertype<glm::vec3>("vec3", sol::constructors<glm::vec3(), glm::vec3(float, float, float)>(), "x", &glm::vec3::x, "y", &glm::vec3::y, "z", &glm::vec3::z);
		lua.set_function("vec3", [](float x, float y, float z) { return glm::vec3(x, y, z); });

		// Bind glm::quat (if you want to access rotations directly)
		if (!lua["quat"].valid()) {
			lua.new_usertype<glm::quat>("quat", sol::constructors<glm::quat(), glm::quat(float, float, float, float)>(), "x", &glm::quat::x, "y", &glm::quat::y, "z", &glm::quat::z, "w", &glm::quat::w);
		}

		lua.new_usertype<Transform>("Transform",
		                            "position",
		                            &Transform::position,
		                            "rotation",
		                            &Transform::rotation,
		                            "scale",
		                            &Transform::scale,

		                            "SetRotation",
		                            &Transform::SetRotation,
		                            "GetEulerAngles",
		                            &Transform::GetEulerAngles);
	}
} // namespace Engine::Components