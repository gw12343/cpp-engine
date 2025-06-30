//
// Created by gabe on 6/30/25.
//

#ifndef CPP_ENGINE_RIGIDBODYCOMPONENT_H
#define CPP_ENGINE_RIGIDBODYCOMPONENT_H

#include "physics/PhysicsManager.h"

namespace Engine::Components {
	class RigidBodyComponent : public Component {
	  public:
		JPH::BodyID bodyID;

		RigidBodyComponent() = default;
		~RigidBodyComponent() override;
		explicit RigidBodyComponent(const JPH::BodyID& bodyID) : bodyID(bodyID) {}

		void OnAdded(Entity& entity) override;
		void RenderInspector(Entity& entity) override;

		static void AddBindings();

		void SetCollisionShape(const JPH::ShapeSettings& settings);
		void SetCollisionShapeRef(const JPH::ShapeRefC& shape);

		// Useful manipulation methods
		glm::vec3 GetPosition() const;
		void      SetPosition(const glm::vec3& position);

		void SetRotation(const glm::quat& rotation);

		void      SetLinearVelocity(const glm::vec3& velocity);
		void      AddLinearVelocity(const glm::vec3& velocity);
		glm::vec3 GetLinearVelocity() const;

		void      SetAngularVelocity(const glm::vec3& velocity);
		glm::vec3 GetAngularVelocity() const;

		void ApplyForce(const glm::vec3& force);
		void ApplyImpulse(const glm::vec3& impulse);

		void ApplyTorque(const glm::vec3& torque);
		void ApplyAngularImpulse(const glm::vec3& impulse);

		void  SetGravityFactor(float factor);
		float GetGravityFactor() const;

		void Activate();
		void Deactivate();
		bool IsActive() const;

		void SetKinematic(bool enable);
		bool IsKinematic() const;


		[[maybe_unused]] void SetSphereShape(const SphereShapeSettings& settings);
		[[maybe_unused]] void SetBoxShape(const BoxShapeSettings& settings);
		[[maybe_unused]] void SetCapsuleShape(const CapsuleShapeSettings& settings);
		[[maybe_unused]] void SetCylinderShape(const CylinderShapeSettings& settings);
		[[maybe_unused]] void SetTriangleShape(const TriangleShapeSettings& settings);

	  private:
		// Conversion utilities
		[[maybe_unused]] static JPH::Vec3 ToJolt(const glm::vec3& v);
		[[maybe_unused]] static glm::vec3 ToGlm(const JPH::Vec3& v);
		[[maybe_unused]] static JPH::Quat ToJolt(const glm::quat& q);
		[[maybe_unused]] static glm::quat ToGlm(const JPH::Quat& q);
	};
} // namespace Engine::Components

#endif // CPP_ENGINE_RIGIDBODYCOMPONENT_H
