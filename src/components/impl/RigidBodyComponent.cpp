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
#include "physics/PhysicsManager.h"
#include "RigidBodyComponent.h"

namespace Engine::Components {


	RigidBodyComponent::~RigidBodyComponent()
	{
		GetPhysics().bodyToEntityMap.erase(bodyID);
	}

	void RigidBodyComponent::OnAdded(Entity& entity)
	{
		BodyInterface& body_interface = GetPhysics().GetPhysicsSystem()->GetBodyInterface();

		if (!body_interface.IsAdded(bodyID)) {
			JPH::Ref<JPH::Shape> shape;
			if (shapeType == "Box") {
				shape = new JPH::BoxShape(shapeSize);
			}
			else if (shapeType == "Sphere") {
				shape = new JPH::SphereShape(shapeSize.GetX());
			}
			else if (shapeType == "Capsule") {
				shape = new JPH::CapsuleShape(shapeSize.GetX(), shapeSize.GetY());
			}
			else if (shapeType == "Cylinder") {
				shape = new JPH::CylinderShape(shapeSize.GetX(), shapeSize.GetY());
			}
			else if (shapeType == "Triangle") {
				// shape = new JPH::CylinderShape(shapeSize.GetX(), shapeSize.GetY());
				// TODO save triangle collider
			}
			else {
				// default to box
				shape = new JPH::BoxShape(shapeSize);
			}

			RVec3 startPos(0, 0, 0);
			Quat  startRot = Quat::sIdentity();
			SPDLOG_INFO("init rb");
			if (entity.HasComponent<Transform>()) {
				SPDLOG_INFO("init rb1");
				auto&     tr  = entity.GetComponent<Transform>();
				glm::vec3 pos = tr.position;
				startPos      = Vec3(pos.x, pos.y, pos.z);
				startRot      = ToJolt(tr.rotation);
			}

			JPH::BodyCreationSettings settings(shape,
			                                   startPos,
			                                   startRot,
			                                   (JPH::EMotionType) motionType,
			                                   Layers::MOVING // replace with your layer definition
			);

			auto                physics       = GetPhysics().GetPhysicsSystem();
			JPH::BodyInterface& bodyInterface = physics->GetBodyInterface();

			settings.mMassPropertiesOverride.mMass = mass;
			settings.mFriction                     = friction;
			settings.mRestitution                  = restitution;
			bodyID                                 = bodyInterface.CreateAndAddBody(settings, JPH::EActivation::Activate);
		}

		GetPhysics().bodyToEntityMap[bodyID] = entity;
	}


	void RigidBodyComponent::RenderInspector(Entity& entity)
	{
		ImGui::Text("Body ID: %u", bodyID.GetIndex());
		// Could add more physics properties here
	}


	void RigidBodyComponent::AddBindings()
	{
		auto& lua = GetScriptManager().lua;


		lua.new_usertype<RigidBodyComponent>("RigidBodyComponent",
		                                     "getPosition",
		                                     &RigidBodyComponent::GetPosition,
		                                     "setPosition",
		                                     &RigidBodyComponent::SetPosition,
		                                     "setRotation",
		                                     &RigidBodyComponent::SetRotation,
		                                     "getLinearVelocity",
		                                     &RigidBodyComponent::GetLinearVelocity,
		                                     "addLinearVelocity",
		                                     &RigidBodyComponent::AddLinearVelocity,
		                                     "setLinearVelocity",
		                                     &RigidBodyComponent::SetLinearVelocity,
		                                     "getAngularVelocity",
		                                     &RigidBodyComponent::GetAngularVelocity,
		                                     "setAngularVelocity",
		                                     &RigidBodyComponent::SetAngularVelocity,
		                                     "applyForce",
		                                     &RigidBodyComponent::ApplyForce,
		                                     "applyImpulse",
		                                     &RigidBodyComponent::ApplyImpulse,
		                                     "applyTorque",
		                                     &RigidBodyComponent::ApplyTorque,
		                                     "applyAngularImpulse",
		                                     &RigidBodyComponent::ApplyAngularImpulse,
		                                     "setGravityFactor",
		                                     &RigidBodyComponent::SetGravityFactor,
		                                     "getGravityFactor",
		                                     &RigidBodyComponent::GetGravityFactor,
		                                     "activate",
		                                     &RigidBodyComponent::Activate,
		                                     "deactivate",
		                                     &RigidBodyComponent::Deactivate,
		                                     "isActive",
		                                     &RigidBodyComponent::IsActive,
		                                     "setKinematic",
		                                     &RigidBodyComponent::SetKinematic,
		                                     "isKinematic",
		                                     &RigidBodyComponent::IsKinematic,

		                                     "setCollisionShape",
		                                     &RigidBodyComponent::SetCollisionShape,
		                                     "setCollisionShapeRef",
		                                     &RigidBodyComponent::SetCollisionShapeRef,

		                                     "setBoxShape",
		                                     &RigidBodyComponent::SetBoxShape,
		                                     "setSphereShape",
		                                     &RigidBodyComponent::SetSphereShape,
		                                     "setCapsuleShape",
		                                     &RigidBodyComponent::SetCapsuleShape,
		                                     "setCylinderShape",
		                                     &RigidBodyComponent::SetCylinderShape,
		                                     "setTriangleShape",
		                                     &RigidBodyComponent::SetTriangleShape);
	}


	// === Conversion Utilities ===
	JPH::Vec3 RigidBodyComponent::ToJolt(const glm::vec3& v)
	{
		return {v.x, v.y, v.z};
	}

	glm::vec3 RigidBodyComponent::ToGlm(const JPH::Vec3& v)
	{
		return glm::vec3(v.GetX(), v.GetY(), v.GetZ());
	}

	JPH::Quat RigidBodyComponent::ToJolt(const glm::quat& q)
	{
		return JPH::Quat(q.x, q.y, q.z, q.w);
	}

	glm::quat RigidBodyComponent::ToGlm(const JPH::Quat& q)
	{
		return glm::quat(q.GetW(), q.GetX(), q.GetY(), q.GetZ()); // glm uses w first
	}

	// === API ===

	void RigidBodyComponent::SetCollisionShape(const JPH::ShapeSettings& settings)
	{
		GetPhysics().log->debug("SETT SetCollisionShape");
		JPH::Shape::ShapeResult result = settings.Create();
		if (result.HasError()) {
			GetPhysics().log->error("Shape creation failed: " + result.GetError());
		}
		SetCollisionShapeRef(result.Get());
	}

	void RigidBodyComponent::SetCollisionShapeRef(const JPH::ShapeRefC& shape)
	{
		auto& bodyInterface = GetPhysics().GetPhysicsSystem()->GetBodyInterface();
		bodyInterface.SetShape(bodyID, shape, true, JPH::EActivation::Activate);

		if (shape.GetPtr()->GetSubType() == EShapeSubType::Box) {
			shapeType             = "Box";
			const auto* box_shape = static_cast<const BoxShape*>(shape.GetPtr());
			shapeSize             = box_shape->GetHalfExtent();
		}
		else if (shape.GetPtr()->GetSubType() == EShapeSubType::Sphere) {
			shapeType                = "Sphere";
			const auto* sphere_shape = static_cast<const SphereShape*>(shape.GetPtr());
			shapeSize                = Vec3(sphere_shape->GetRadius(), 0.0, 0.0);
		}
		else if (shape.GetPtr()->GetSubType() == EShapeSubType::Cylinder) {
			shapeType                  = "Cylinder";
			const auto* cylinder_shape = static_cast<const CylinderShape*>(shape.GetPtr());
			shapeSize                  = Vec3(cylinder_shape->GetHalfHeight(), cylinder_shape->GetRadius(), 0.0);
		}
		else if (shape.GetPtr()->GetSubType() == EShapeSubType::Capsule) {
			shapeType                 = "Capsule";
			const auto* capsule_shape = static_cast<const CapsuleShape*>(shape.GetPtr());
			shapeSize                 = Vec3(capsule_shape->GetHalfHeightOfCylinder(), capsule_shape->GetRadius(), 0.0);
		}
		else if (shape.GetPtr()->GetSubType() == EShapeSubType::Triangle) {
			// TODO triangle
		}
		else {
			shapeType = "Box";
		}
	}


	void RigidBodyComponent::SetBoxShape(const JPH::BoxShapeSettings& settings)
	{
		auto result = settings.Create();
		if (result.HasError()) {
			GetPhysics().log->error("BoxShape creation failed: " + result.GetError());
		}
		SetCollisionShapeRef(result.Get());
		shapeType = "Box";
		shapeSize = settings.mHalfExtent;
	}

	void RigidBodyComponent::SetSphereShape(const JPH::SphereShapeSettings& settings)
	{
		auto result = settings.Create();
		if (result.HasError()) {
			GetPhysics().log->error("SphereShape creation failed: " + result.GetError());
		}
		SetCollisionShapeRef(result.Get());
		shapeType = "Sphere";
		shapeSize = Vec3(settings.mRadius, 0.0, 0.0);
	}

	void RigidBodyComponent::SetCapsuleShape(const JPH::CapsuleShapeSettings& settings)
	{
		auto result = settings.Create();
		if (result.HasError()) {
			GetPhysics().log->error("CapsuleShape creation failed: " + result.GetError());
		}
		SetCollisionShapeRef(result.Get());
		shapeType = "Capsule";
		shapeSize = Vec3(settings.mHalfHeightOfCylinder, settings.mRadius, 0.0);
	}

	void RigidBodyComponent::SetCylinderShape(const JPH::CylinderShapeSettings& settings)
	{
		auto result = settings.Create();
		if (result.HasError()) {
			GetPhysics().log->error("CylinderShape creation failed: " + result.GetError());
		}
		SetCollisionShapeRef(result.Get());
		shapeType = "Cylinder";
		shapeSize = Vec3(settings.mHalfHeight, settings.mRadius, 0.0);
	}

	void RigidBodyComponent::SetTriangleShape(const JPH::TriangleShapeSettings& settings)
	{
		auto result = settings.Create();
		if (result.HasError()) {
			GetPhysics().log->error("TriangleShape creation failed: " + result.GetError());
		}
		SetCollisionShapeRef(result.Get());
		shapeType = "Triangle";
	}


	glm::vec3 RigidBodyComponent::GetPosition() const
	{
		return ToGlm(GetPhysics().GetPhysicsSystem()->GetBodyInterface().GetPosition(bodyID));
	}

	void RigidBodyComponent::SetPosition(const glm::vec3& position)
	{
		GetPhysics().GetPhysicsSystem()->GetBodyInterface().SetPosition(bodyID, ToJolt(position), JPH::EActivation::Activate);
	}

	void RigidBodyComponent::SetRotation(const glm::quat& rotation)
	{
		GetPhysics().GetPhysicsSystem()->GetBodyInterface().SetRotation(bodyID, ToJolt(rotation), JPH::EActivation::Activate);
	}

	void RigidBodyComponent::SetLinearVelocity(const glm::vec3& velocity)
	{
		GetPhysics().GetPhysicsSystem()->GetBodyInterface().SetLinearVelocity(bodyID, ToJolt(velocity));
	}

	void RigidBodyComponent::AddLinearVelocity(const glm::vec3& velocity)
	{
		GetPhysics().GetPhysicsSystem()->GetBodyInterface().AddLinearVelocity(bodyID, ToJolt(velocity));
	}

	glm::vec3 RigidBodyComponent::GetLinearVelocity() const
	{
		return ToGlm(GetPhysics().GetPhysicsSystem()->GetBodyInterface().GetLinearVelocity(bodyID));
	}

	void RigidBodyComponent::SetAngularVelocity(const glm::vec3& velocity)
	{
		GetPhysics().GetPhysicsSystem()->GetBodyInterface().SetAngularVelocity(bodyID, ToJolt(velocity));
	}

	glm::vec3 RigidBodyComponent::GetAngularVelocity() const
	{
		return ToGlm(GetPhysics().GetPhysicsSystem()->GetBodyInterface().GetAngularVelocity(bodyID));
	}

	void RigidBodyComponent::ApplyForce(const glm::vec3& force)
	{
		GetPhysics().GetPhysicsSystem()->GetBodyInterface().AddForce(bodyID, ToJolt(force));
	}

	void RigidBodyComponent::ApplyImpulse(const glm::vec3& impulse)
	{
		GetPhysics().GetPhysicsSystem()->GetBodyInterface().AddImpulse(bodyID, ToJolt(impulse));
	}

	void RigidBodyComponent::ApplyTorque(const glm::vec3& torque)
	{
		GetPhysics().GetPhysicsSystem()->GetBodyInterface().AddTorque(bodyID, ToJolt(torque));
	}

	void RigidBodyComponent::ApplyAngularImpulse(const glm::vec3& impulse)
	{
		GetPhysics().GetPhysicsSystem()->GetBodyInterface().AddAngularImpulse(bodyID, ToJolt(impulse));
	}

	void RigidBodyComponent::SetGravityFactor(float factor)
	{
		GetPhysics().GetPhysicsSystem()->GetBodyInterface().SetGravityFactor(bodyID, factor);
		gravityFactor = factor;
	}

	float RigidBodyComponent::GetGravityFactor() const
	{
		return GetPhysics().GetPhysicsSystem()->GetBodyInterface().GetGravityFactor(bodyID);
	}

	void RigidBodyComponent::Activate()
	{
		GetPhysics().GetPhysicsSystem()->GetBodyInterface().ActivateBody(bodyID);
	}

	void RigidBodyComponent::Deactivate()
	{
		GetPhysics().GetPhysicsSystem()->GetBodyInterface().DeactivateBody(bodyID);
	}

	bool RigidBodyComponent::IsActive() const
	{
		return GetPhysics().GetPhysicsSystem()->GetBodyInterface().IsActive(bodyID);
	}

	void RigidBodyComponent::SetKinematic(bool enable)
	{
		GetPhysics().GetPhysicsSystem()->GetBodyInterface().SetMotionType(bodyID, enable ? JPH::EMotionType::Kinematic : JPH::EMotionType::Dynamic, JPH::EActivation::DontActivate);
		motionType = enable ? (int) JPH::EMotionType::Kinematic : (int) JPH::EMotionType::Dynamic;
	}

	bool RigidBodyComponent::IsKinematic() const
	{
		return GetPhysics().GetPhysicsSystem()->GetBodyInterface().GetMotionType(bodyID) == JPH::EMotionType::Kinematic;
	}


} // namespace Engine::Components