//
// Created by gabe on 6/30/25.
//

#ifndef CPP_ENGINE_TRANSFORMCOMPONENT_H
#define CPP_ENGINE_TRANSFORMCOMPONENT_H


#include "components/Components.h"
#include "glm/detail/type_quat.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"

#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/array.hpp>

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

			template <class Archive>
			void serialize(Archive& ar)
			{
				ar(cereal::make_nvp("position", position), cereal::make_nvp("rotation", rotation), cereal::make_nvp("scale", scale));
			}


			//			Transform(const Transform& copy)
			//			{
			//				position = copy.position;
			//				rotation = copy.rotation;
			//				scale    = copy.scale;
			//			}

			explicit Transform(const glm::vec3& position) : position(position) {}

			explicit Transform(const glm::vec3& position, const glm::vec3& eulerAngles = glm::vec3(0.0f), const glm::vec3& scale = glm::vec3(1.0f)) : position(position), rotation(glm::quat(glm::radians(eulerAngles))), scale(scale) {}

			void SyncWithPhysics(Entity& entity);

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
			void OnRemoved(Entity& entity) override;
			void RenderInspector(Entity& entity) override;

			static void AddBindings();
		};
	} // namespace Components
} // namespace Engine

#endif // CPP_ENGINE_TRANSFORMCOMPONENT_H
