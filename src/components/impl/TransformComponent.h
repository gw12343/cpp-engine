//
// Created by gabe on 6/30/25.
//

#ifndef CPP_ENGINE_TRANSFORMCOMPONENT_H
#define CPP_ENGINE_TRANSFORMCOMPONENT_H


#include "components/Components.h"
#include "glm/detail/type_quat.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"

namespace Engine {
	class Mesh;
	class Entity;
	namespace Components {
		// Transform component for positioning, rotating, and scaling entities
		class Transform : public Component {
		  public:
			glm::vec3 position = glm::vec3(0.0f);
			glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // Identity quaternion
			glm::vec3 scale    = glm::vec3(1.0f);

			Transform() = default;

			explicit Transform(const glm::vec3& position) : position(position) {}

			explicit Transform(const glm::vec3& position, const glm::vec3& eulerAngles = glm::vec3(0.0f), const glm::vec3& scale = glm::vec3(1.0f)) : position(position), rotation(glm::quat(glm::radians(eulerAngles))), scale(scale) {}

			// Set rotation using Euler angles (in degrees)
			void SetRotation(const glm::vec3& eulerAngles) { rotation = glm::quat(glm::radians(eulerAngles)); }

			// Get rotation as Euler angles (in degrees)
			[[nodiscard]] glm::vec3 GetEulerAngles() const { return glm::degrees(glm::eulerAngles(rotation)); }

			// Get the transformation matrix
			[[nodiscard]] glm::mat4 GetMatrix() const
			{
				auto matrix = glm::mat4(1.0f);
				matrix      = glm::translate(matrix, position);
				matrix      = matrix * glm::toMat4(rotation);
				matrix      = glm::scale(matrix, scale);
				return matrix;
			}

			void OnAdded(Entity& entity) override;
			void RenderInspector(Entity& entity) override;

			static void AddBindings();
		};
	} // namespace Components
} // namespace Engine

#endif // CPP_ENGINE_TRANSFORMCOMPONENT_H
