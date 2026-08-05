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
	};
} // namespace Engine::Components

#endif // CPP_ENGINE_PLAYERCONTROLLERCOMPONENT_H
