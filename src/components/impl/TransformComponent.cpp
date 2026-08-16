//
// Created by gabe on 6/24/25.
//
#include "components/Components.h"
#include "core/Entity.h"


#include "ozz/animation/runtime/track.h"
#include "rendering/particles/ParticleManager.h"
#include "animation/AnimationManager.h"
#include "scripting/ScriptManager.h"

#include "physics/PhysicsManager.h"
#include "TransformComponent.h"
#include "RigidBodyComponent.h"
#include "PlayerControllerComponent.h"

#include "EntityMetadataComponent.h"
#include "glm/gtx/matrix_decompose.inl"

namespace Engine::Components {
	void Transform::OnRemoved(Entity& entity)
	{
	}
	void Transform::ExtractTRS(const glm::mat4& m, glm::vec3& translation, glm::quat& rotation, glm::vec3& scale)
	{
		translation = glm::vec3(m[3]);

		const float sx = glm::length(glm::vec3(m[0]));
		const float sy = glm::length(glm::vec3(m[1]));
		const float sz = glm::length(glm::vec3(m[2]));
		scale          = glm::vec3(sx, sy, sz);

		glm::mat3 rot(1.0f);
		const float eps = 1e-8f;
		rot[0]          = (sx > eps) ? (glm::vec3(m[0]) / sx) : glm::vec3(1.f, 0.f, 0.f);
		rot[1]          = (sy > eps) ? (glm::vec3(m[1]) / sy) : glm::vec3(0.f, 1.f, 0.f);
		rot[2]          = (sz > eps) ? (glm::vec3(m[2]) / sz) : glm::vec3(0.f, 0.f, 1.f);

		// Negative determinant means a reflection; fold it into scale so quat_cast
		// sees a proper rotation.
		if (glm::determinant(rot) < 0.0f) {
			scale.x = -scale.x;
			rot[0]  = -rot[0];
		}

		rotation = glm::normalize(glm::quat_cast(rot));
		// q and -q are the same rotation; keep w >= 0 so serialization is stable.
		if (rotation.w < 0.0f) {
			rotation = -rotation;
		}
	}

	void Transform::SetLocalFromWorld(const glm::mat4& parentWorld, const glm::vec3& worldPos, const glm::quat& worldRot, const glm::vec3& worldScale)
	{
		const glm::mat4 localMatrix = glm::inverse(parentWorld) * ComposeTRS(worldPos, worldRot, worldScale);
		ExtractTRS(localMatrix, localPosition, localRotation, localScale);
	}

	void Transform::SetWorldFromMatrix(const glm::mat4& world)
	{
		worldMatrix = world;
		ExtractTRS(world, worldPosition, worldRotation, worldScale);
	}

	void Transform::OnAdded(Entity& entity)
	{
		worldPosition = localPosition;
		worldRotation = localRotation;
		worldScale    = localScale;
		worldMatrix   = GetLocalMatrix();
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

		static bool         showWorld = false;
		static entt::entity eulerEnt  = entt::null;
		static glm::vec3    cachedEuler(0.0f);

		ImGui::Checkbox("World Space", &showWorld);

		glm::vec3& pos   = showWorld ? worldPosition : localPosition;
		glm::quat  rot   = showWorld ? GetWorldRotation() : GetLocalRotation();
		glm::vec3& scale = showWorld ? worldScale : localScale;

		glm::vec3 displayPos = pos;
		ImGui::PushID("pos");
		if (LeftLabelDragFloat3("Position", glm::value_ptr(displayPos), 0.1f)) {
			pos                           = displayPos;
			updatePhysicsPositionManually = true;
		}
		ImGui::SameLine();
		if (ImGui::SmallButton("0##pos")) {
			pos                           = glm::vec3(0.0f);
			updatePhysicsPositionManually = true;
		}
		ImGui::PopID();

		if (entity.GetENTTHandle() != eulerEnt || !ImGui::IsItemActive()) {
			if (entity.GetENTTHandle() != eulerEnt) {
				eulerEnt     = entity.GetENTTHandle();
				cachedEuler  = glm::degrees(glm::eulerAngles(rot));
			}
			else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
				cachedEuler = glm::degrees(glm::eulerAngles(rot));
			}
		}

		ImGui::PushID("rot");
		if (LeftLabelDragFloat3("Rotation", glm::value_ptr(cachedEuler), 1.0f)) {
			rot                           = glm::quat(glm::radians(cachedEuler));
			if (showWorld) SetWorldRotation(rot);
			else SetLocalRotation(rot);
			updatePhysicsPositionManually = true;
		}
		ImGui::SameLine();
		if (ImGui::SmallButton("0##rot")) {
			rot                           = glm::quat(1, 0, 0, 0);
			cachedEuler                   = glm::vec3(0.0f);
			if (showWorld) SetWorldRotation(rot);
			else SetLocalRotation(rot);
			updatePhysicsPositionManually = true;
		}
		ImGui::PopID();

		ImGui::PushID("scl");
		if (LeftLabelDragFloat3("Scale", glm::value_ptr(scale), 0.1f)) {
			updatePhysicsPositionManually = true;
		}
		ImGui::SameLine();
		if (ImGui::SmallButton("1##scl")) {
			scale                         = glm::vec3(1.0f);
			updatePhysicsPositionManually = true;
		}
		ImGui::PopID();

		if (showWorld && updatePhysicsPositionManually) {
			entity.SetWorldTransform(worldPosition, GetWorldRotation(), worldScale);
			updatePhysicsPositionManually = true;
		}

		if (updatePhysicsPositionManually) {
			auto& em           = entity.GetComponent<EntityMetadata>();
			auto  parentMatrix = glm::mat4(1.0);

			if (em.parentEntity.IsValid()) {
				Entity parent = GetCurrentScene()->Get(em.parentEntity);
				if (parent && parent.HasComponent<Transform>()) {
					auto& parentTr = parent.GetComponent<Transform>();
					parentMatrix   = parentTr.GetWorldMatrix();
				}
			}

			SetWorldFromMatrix(parentMatrix * GetLocalMatrix());
			SyncWithPhysics(entity);
		}
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