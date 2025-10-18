//
// Created by gabe on 8/22/25.
//

#include "PlayerControllerComponent.h"
#include "core/EngineData.h"
#include "physics/PhysicsManager.h"
#include "imgui.h"
#include "scripting/ScriptManager.h"

namespace Engine::Components {
	void PlayerControllerComponent::OnAdded(Engine::Entity& entity)
	{
		if (entity.HasComponent<Components::Transform>()) {
			auto& tr = entity.GetComponent<Components::Transform>();
			SetPosition(tr.position);
			SetRotation(tr.rotation);
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
		auto rot = GetPhysics().GetCharacter()->GetRotation();
		return {rot.GetW(), rot.GetX(), rot.GetY(), rot.GetZ()};
	}
	void PlayerControllerComponent::SetRotation(glm::quat rot)
	{
		Quat joltRot = Quat(rot.x, rot.y, rot.z, rot.w);
		GetPhysics().GetCharacter()->SetRotation(joltRot);
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
		                                            &PlayerControllerComponent::SetRotationEuler);
	}

	void PlayerControllerComponent::SetLinearVelocity(glm::vec3 vel)
	{
		RVec3 joltVel = RVec3(vel.x, vel.y, vel.z);
		GetPhysics().GetCharacter()->SetLinearVelocity(joltVel);
	}

	void PlayerControllerComponent::SetRotationEuler(glm::vec3 eulerAngles)
	{
		glm::quat rot = glm::quat(glm::radians(eulerAngles));
		SetRotation(rot);
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
