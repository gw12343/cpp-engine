//
// Created by gabe on 8/22/25.
//

#include "PlayerControllerComponent.h"
#include "core/EngineData.h"
#include "physics/PhysicsManager.h"
#include "physics/PlayerController.h"
#include "physics/PlayerSettings.h"

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
		SetClimbing(false);
	}
	void PlayerControllerComponent::RenderInspector(Engine::Entity& entity)
	{
		auto pos = GetPosition();
		ImGui::Text("Position: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
		ImGui::Text("On Ground: %s", IsOnGround() ? "yes" : "no");
		ImGui::Text("Climbing: %s", IsClimbing() ? "yes" : "no");
		ImGui::Text("Capsule: r=%.2f  halfH=%.2f", GetCapsuleRadius(), GetCapsuleHalfHeight());
		if (mHasClimbSurface) {
			ImGui::Text("Climb N: (%.2f, %.2f, %.2f)", mClimbNormal.x, mClimbNormal.y, mClimbNormal.z);
		}
		ImGui::TextDisabled("Move / slope settings live in PlayerSettings.h");
		(void) entity;
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

	void PlayerControllerComponent::SetClimbing(bool climbing)
	{
		if (auto* pc = GetPhysics().GetPlayerController()) {
			pc->SetClimbing(climbing);
		}
		if (!climbing) {
			mHasClimbSurface = false;
		}
	}

	bool PlayerControllerComponent::IsClimbing() const
	{
		if (const auto* pc = GetPhysics().GetPlayerController()) {
			return pc->IsClimbing();
		}
		return false;
	}

	float PlayerControllerComponent::GetCapsuleRadius() const
	{
		return cCharacterRadius;
	}

	float PlayerControllerComponent::GetCapsuleHalfHeight() const
	{
		return cCharacterHalfHeight;
	}

	bool PlayerControllerComponent::ProbeClimbSurface(glm::vec3 worldDir, float maxDistance, float minNormalY, float maxNormalY)
	{
		mHasClimbSurface = false;

		const float len2 = glm::dot(worldDir, worldDir);
		if (len2 < 1e-12f || maxDistance <= 0.f) {
			return false;
		}
		worldDir *= glm::inversesqrt(len2);

		const glm::vec3 body = GetPosition();
		// Chest-height probe origin (capsule center is body position).
		const glm::vec3 origin = body + glm::vec3(0.f, 0.15f, 0.f);

		glm::vec3 hitPoint{}, hitNormal{};
		float     hitDist = 0.f;
		if (!GetPhysics().Raycast(origin, worldDir, maxDistance, hitPoint, hitNormal, hitDist)) {
			return false;
		}

		// Outward normal: point away from surface toward free space (toward the character).
		if (glm::dot(hitNormal, worldDir) > 0.f) {
			hitNormal = -hitNormal;
		}

		// Climbable slope band: steep walls (low |n.y|) but not ceilings.
		// minNormalY ~ -0.2 allows mild overhangs; maxNormalY ~ 0.55 excludes walkable floors.
		if (hitNormal.y < minNormalY || hitNormal.y > maxNormalY) {
			return false;
		}

		mClimbPoint      = hitPoint;
		mClimbNormal     = hitNormal;
		mHasClimbSurface = true;
		return true;
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
		                                            &PlayerControllerComponent::SetFacingDirection,

		                                            "setClimbing",
		                                            &PlayerControllerComponent::SetClimbing,
		                                            "isClimbing",
		                                            &PlayerControllerComponent::IsClimbing,
		                                            "getCapsuleRadius",
		                                            &PlayerControllerComponent::GetCapsuleRadius,
		                                            "getCapsuleHalfHeight",
		                                            &PlayerControllerComponent::GetCapsuleHalfHeight,
		                                            "getClimbNormal",
		                                            &PlayerControllerComponent::GetClimbNormal,
		                                            "getClimbPoint",
		                                            &PlayerControllerComponent::GetClimbPoint,
		                                            "hasClimbSurface",
		                                            &PlayerControllerComponent::HasClimbSurface,
		                                            "probeClimbSurface",
		                                            sol::overload(
		                                                [](PlayerControllerComponent& self, const glm::vec3& dir, float maxDist) {
			                                                // Defaults: mild overhang OK, reject floors shallower than ~57°.
			                                                return self.ProbeClimbSurface(dir, maxDist, -0.25f, 0.55f);
		                                                },
		                                                [](PlayerControllerComponent& self, const glm::vec3& dir, float maxDist, float minNy, float maxNy) {
			                                                return self.ProbeClimbSurface(dir, maxDist, minNy, maxNy);
		                                                }));
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
