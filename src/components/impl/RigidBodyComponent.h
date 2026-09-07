//
// Created by gabe on 6/30/25.
//

#ifndef CPP_ENGINE_RIGIDBODYCOMPONENT_H
#define CPP_ENGINE_RIGIDBODYCOMPONENT_H

#include "assets/AssetHandle.h"
#include "components/Components.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/MotionType.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>

namespace Engine::Components {
	class RigidBodyComponent : public Component {
	  public:
		JPH::BodyID bodyID;

		int         motionType    = (int) JPH::EMotionType::Dynamic;
		float       mass          = 1.0f;
		float       friction      = 0.5f;
		float       restitution   = 0.0f;
		float       gravityFactor = 1.0f;
		std::string shapeType     = "Box";
		JPH::Vec3   shapeSize     = JPH::Vec3::sReplicate(1.0f);
		std::vector<bool> meshSelection;
		JPH::Vec3         centerOfMassOffset = JPH::Vec3::sZero();
		ModelHandle       colliderModel;

		RigidBodyComponent() : bodyID(0) {}

		explicit RigidBodyComponent(const JPH::BodyID& bodyID) : bodyID(bodyID) {}
		RigidBodyComponent(const RigidBodyComponent& other);

		template <class Archive>
		void serialize(Archive& ar)
		{
			ar(CEREAL_NVP(motionType), CEREAL_NVP(mass), CEREAL_NVP(friction), CEREAL_NVP(restitution), CEREAL_NVP(gravityFactor), CEREAL_NVP(shapeType), CEREAL_NVP(shapeSize), CEREAL_NVP(meshSelection), CEREAL_NVP(colliderModel));
		}

		void OnAdded(Entity& entity) override;
		void OnRemoved(Entity& entity) override;
		void RenderInspector(Entity& entity) override;

		static void AddBindings();

		void SetCollisionShape(const JPH::ShapeSettings& settings);
		void SetCollisionShapeRef(const JPH::ShapeRefC& shape);

		glm::vec3 GetPosition() const;
		void      MoveKinematic(const glm::vec3& position, const glm::quat& rotation, float dt);
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

		[[maybe_unused]] void SetSphereShape(const JPH::SphereShapeSettings& settings);
		[[maybe_unused]] void SetBoxShape(const JPH::BoxShapeSettings& settings);
		[[maybe_unused]] void SetCapsuleShape(const JPH::CapsuleShapeSettings& settings);
		[[maybe_unused]] void SetCylinderShape(const JPH::CylinderShapeSettings& settings);
		[[maybe_unused]] void SetMeshShape(Entity& entity);
		[[maybe_unused]] void SetConvexMeshShape(Entity& entity);

		void SetRotationEuler(const glm::vec3& eulerAngles);

	  public:
		[[maybe_unused]] static JPH::Vec3 ToJolt(const glm::vec3& v);
		[[maybe_unused]] static glm::vec3 ToGlm(const JPH::Vec3& v);
		[[maybe_unused]] static JPH::Quat ToJolt(const glm::quat& q);
		[[maybe_unused]] static glm::quat ToGlm(const JPH::Quat& q);
	};
} // namespace Engine::Components

#endif // CPP_ENGINE_RIGIDBODYCOMPONENT_H
