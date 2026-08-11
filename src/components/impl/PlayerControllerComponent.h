//
// Created by gabe on 8/22/25.
//

#ifndef CPP_ENGINE_PLAYERCONTROLLERCOMPONENT_H
#define CPP_ENGINE_PLAYERCONTROLLERCOMPONENT_H

#include "components/Components.h"

namespace Engine::Components {
	class PlayerControllerComponent : public Component {
	  public:
		template <class Archive>
		void serialize(Archive& ar)
		{
		}

		PlayerControllerComponent() = default;

		void        OnAdded(Entity& entity) override;
		void        OnRemoved(Entity& entity) override;
		void        RenderInspector(Entity& entity) override;
		glm::vec3   GetPosition();
		glm::vec3   GetLinearVelocity();
		glm::vec3   GetGroundVelocity();
		void        SetPosition(glm::vec3 pos);
		void        SetLinearVelocity(glm::vec3 vel);
		glm::quat   GetRotation();
		void        SetRotation(glm::quat rot);
		bool        IsOnGround();
		static void AddBindings();
		void        SetRotationEuler(glm::vec3 eulerAngles);

		// Face a world-space direction on XZ (y ignored). Uses the same yaw basis as
		// the engine camera: flat forward = (cos yaw, sin yaw).
		void SetFacingDirection(glm::vec3 worldDir);

		// --- Climbing (BOTW-style wall attach; physics only) ---
		void SetClimbing(bool climbing);
		bool IsClimbing() const;

		/// Capsule radius used for wall stick distance (matches CharacterVirtual shape).
		float GetCapsuleRadius() const;
		/// Cylinder half-height (not full height) of the standing capsule.
		float GetCapsuleHalfHeight() const;

		/// Last successful climb probe normal / point (updated by ProbeClimbSurface).
		glm::vec3 GetClimbNormal() const { return mClimbNormal; }
		glm::vec3 GetClimbPoint() const { return mClimbPoint; }
		bool      HasClimbSurface() const { return mHasClimbSurface; }

		/// Raycast from chest along worldDir for a climbable surface.
		/// Returns true and fills point/normal/distance when a steep-enough wall is hit.
		bool ProbeClimbSurface(glm::vec3 worldDir, float maxDistance, float minNormalY, float maxNormalY);

	  private:
		glm::vec3 mClimbNormal{0.f, 0.f, 1.f};
		glm::vec3 mClimbPoint{0.f};
		bool      mHasClimbSurface = false;
	};
} // namespace Engine::Components

#endif // CPP_ENGINE_PLAYERCONTROLLERCOMPONENT_H
