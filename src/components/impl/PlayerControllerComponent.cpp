//
// Created by gabe on 8/22/25.
//

#include "PlayerControllerComponent.h"
#include "core/EngineData.h"
#include "physics/PhysicsManager.h"

#include "scripting/ScriptManager.h"
#include <cmath>

namespace Engine::Components {
	void PlayerControllerComponent::OnAdded(Engine::Entity& entity)
	{
		if (entity.HasComponent<Components::Transform>()) {
			auto& tr = entity.GetComponent<Components::Transform>();
			SetPosition(tr.GetWorldPosition());
			SetRotation(tr.GetWorldRotation());
		}
	}
	void PlayerControllerComponent::OnRemoved(Engine::Entity& entity)
	{
	}
	void PlayerControllerComponent::RenderInspector(Engine::Entity& entity)
	{
		auto pos = GetPosition();
		ImGui::Text("Position: (%f, %f, %f)", pos.x, pos.y, pos.z);
	}


	glm::vec3 PlayerControllerComponent::GetPosition()
	{
		auto pos = GetPhysics().GetCharacter()->GetPosition();
		return {pos.GetX(), pos.GetY(), pos.GetZ()};
	}

	void PlayerControllerComponent::SetPosition(glm::vec3 pos)
	{
		RVec3 joltPos = RVec3(pos.x, pos.y, pos.z);
		GetPhysics().GetCharacter()->SetPosition(joltPos);
	}
	glm::quat PlayerControllerComponent::GetRotation()
	{
		// Same conversion as RigidBodyComponent::ToGlm
		const Quat rot = GetPhysics().GetCharacter()->GetRotation();
		return glm::quat(rot.GetW(), rot.GetX(), rot.GetY(), rot.GetZ());
	}
	void PlayerControllerComponent::SetRotation(glm::quat rot)
	{
		// Same conversion as RigidBodyComponent::ToJolt
		rot = glm::normalize(rot);
		GetPhysics().GetCharacter()->SetRotation(Quat(rot.x, rot.y, rot.z, rot.w));
	}
	void PlayerControllerComponent::AddBindings()
	{
		auto& lua = GetScriptManager().lua;
		lua.new_usertype<PlayerControllerComponent>("PlayerControllerComponent",

		                                            "isOnGround",
		                                            &PlayerControllerComponent::IsOnGround,
		                                            "setLinearVelocity",
		                                            &PlayerControllerComponent::SetLinearVelocity,
		                                            "getLinearVelocity",
		                                            &PlayerControllerComponent::GetLinearVelocity,
		                                            "getGroundVelocity",
		                                            &PlayerControllerComponent::GetGroundVelocity,
		                                            "getPosition",
		                                            &PlayerControllerComponent::GetPosition,
		                                            "setPosition",
		                                            &PlayerControllerComponent::SetPosition,
		                                            "setRotation",
		                                            &PlayerControllerComponent::SetRotation,
		                                            "setRotationEuler",
		                                            &PlayerControllerComponent::SetRotationEuler,
		                                            "setFacingDirection",
		                                            &PlayerControllerComponent::SetFacingDirection);
	}

	void PlayerControllerComponent::SetLinearVelocity(glm::vec3 vel)
	{
		RVec3 joltVel = RVec3(vel.x, vel.y, vel.z);
		GetPhysics().GetCharacter()->SetLinearVelocity(joltVel);
	}

	void PlayerControllerComponent::SetRotationEuler(glm::vec3 eulerAngles)
	{
		// .y = degrees of yaw about world +Y. Uses Jolt's rotation helper so the
		// character and rendered transform share one convention.
		const float yawRad = glm::radians(eulerAngles.y);
		GetPhysics().GetCharacter()->SetRotation(Quat::sRotation(Vec3::sAxisY(), yawRad));
	}

	void PlayerControllerComponent::SetFacingDirection(glm::vec3 worldDir)
	{
		// Face so local +Z points along worldDir on the XZ plane.
		// (Mixamo / ozz bind pose forward is +Z.)
		//
		// With Ry(yaw): +Z -> (sin yaw, 0, cos yaw)
		// so yaw = atan2(dir.x, dir.z).
		//
		// Do NOT use atan2(z, x) — that is the camera yaw basis
		// (front = (cos, sin)) and turns the character the wrong way relative
		// to orbit while holding W.
		worldDir.y = 0.f;
		const float len2 = glm::dot(worldDir, worldDir);
		if (len2 < 1e-12f) {
			return;
		}
		worldDir *= glm::inversesqrt(len2);

		const float yawRad = std::atan2(worldDir.x, worldDir.z);
		GetPhysics().GetCharacter()->SetRotation(Quat::sRotation(Vec3::sAxisY(), yawRad));
	}

	glm::vec3 PlayerControllerComponent::GetLinearVelocity()
	{
		auto v = GetPhysics().GetCharacter()->GetLinearVelocity();
		return {v.GetX(), v.GetY(), v.GetZ()};
	}
	glm::vec3 PlayerControllerComponent::GetGroundVelocity()
	{
		auto v = GetPhysics().GetCharacter()->GetGroundVelocity();
		return {v.GetX(), v.GetY(), v.GetZ()};
	}
	bool PlayerControllerComponent::IsOnGround()
	{
		return GetPhysics().GetCharacter()->GetGroundState() == JPH::CharacterBase::EGroundState::OnGround;
	}

} // namespace Engine::Components
