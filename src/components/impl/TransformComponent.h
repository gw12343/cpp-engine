//
// Created by gabe on 6/30/25.
//

#ifndef CPP_ENGINE_TRANSFORMCOMPONENT_H
#define CPP_ENGINE_TRANSFORMCOMPONENT_H


#include "components/Components.h"


#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/array.hpp>


namespace Engine {
	class Entity;
	namespace Components {

		// Transform component for positioning, rotating, and scaling entities (with hierarchy support)
		class Transform : public Component {
		  private:
			// ───────────────────────────────
			// Local-space properties (serialized)
			// ───────────────────────────────
			glm::vec3 localPosition = glm::vec3(0.0f);
			glm::quat localRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // identity quaternion
			glm::vec3 localScale    = glm::vec3(1.0f);

			// ───────────────────────────────
			// World-space (computed, not serialized)
			// ───────────────────────────────
			glm::vec3 worldPosition = glm::vec3(0.0f);
			glm::quat worldRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
			glm::vec3 worldScale    = glm::vec3(1.0f);
			glm::mat4 worldMatrix   = glm::mat4(1.0f);

		  public:
			glm::vec3& GetLocalPosition() { return localPosition; }
			glm::vec3& GetWorldPosition() { return worldPosition; }
			glm::quat  GetLocalRotation() { return glm::normalize(localRotation); }
			glm::quat  GetWorldRotation() { return glm::normalize(worldRotation); }
			glm::vec3& GetLocalScale() { return localScale; }
			glm::vec3& GetWorldScale() { return worldScale; }

			glm::mat4& GetWorldMatrix() { return worldMatrix; }
			void       SetLocalPosition(glm::vec3 newPos) { localPosition = newPos; }
			void       SetWorldPosition(glm::vec3 newPos) { worldPosition = newPos; }
			void       SetLocalRotation(glm::quat newRot) { localRotation = glm::normalize(newRot); }
			void       SetWorldRotation(glm::quat newRot) { worldRotation = glm::normalize(newRot); }
			void       SetLocalScale(glm::vec3 newScale) { localScale = newScale; }
			void       SetWorldScale(glm::vec3 newScale) { worldScale = newScale; }

			void SetWorldMatrix(glm::mat4 newWorldMatrix) { worldMatrix = newWorldMatrix; }

			Transform()                       = default;
			Transform(const Transform& other) = default;

			explicit Transform(const glm::vec3& pos) : localPosition(pos) {}
			explicit Transform(const glm::vec3& pos, const glm::vec3& euler, const glm::vec3& scl = glm::vec3(1.0f)) : localPosition(pos), localRotation(glm::quat(glm::radians(euler))), localScale(scl) {}

			// ───────────────────────────────
			// Serialization
			// ───────────────────────────────
			template <class Archive>
			void serialize(Archive& ar)
			{
				ar(cereal::make_nvp("position", localPosition), cereal::make_nvp("rotation", localRotation), cereal::make_nvp("scale", localScale));
			}

			// ───────────────────────────────
			// Helpers
			// ───────────────────────────────

			// Rotation (Euler degrees)
			void                    SetRotation(const glm::vec3& eulerDegrees) { localRotation = glm::quat(glm::radians(eulerDegrees)); }
			[[nodiscard]] glm::vec3 GetEulerAngles() const { return glm::degrees(glm::eulerAngles(localRotation)); }

			// Local transform matrix
			[[nodiscard]] glm::mat4 GetLocalMatrix() const { return ComposeTRS(localPosition, localRotation, localScale); }

			// T * R * S. Inverse of ExtractTRS for positive scales.
			static glm::mat4 ComposeTRS(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale)
			{
				return glm::translate(glm::mat4(1.0f), translation) * glm::mat4_cast(glm::normalize(rotation)) * glm::scale(glm::mat4(1.0f), scale);
			}

			// Recover T / R / S from a column-major affine matrix.
			// glm::quat_cast on a matrix that still contains scale is wrong — always
			// strip scale (and a possible reflection) from the 3x3 first.
			static void ExtractTRS(const glm::mat4& m, glm::vec3& translation, glm::quat& rotation, glm::vec3& scale);

			// Bake a world-space pose into local TRS given the parent's world matrix.
			void SetLocalFromWorld(const glm::mat4& parentWorld, const glm::vec3& worldPos, const glm::quat& worldRot, const glm::vec3& worldScale);

			// Write world cache from a composed world matrix (used by the hierarchy update).
			void SetWorldFromMatrix(const glm::mat4& world);


			// Sync physics
			void SyncWithPhysics(Entity& entity);

			// ───────────────────────────────
			// Engine lifecycle
			// ───────────────────────────────
			void OnAdded(Entity& entity) override;
			void OnRemoved(Entity& entity) override;
			void RenderInspector(Entity& entity) override;

			static void AddBindings();
		};

	} // namespace Components
} // namespace Engine

#endif // CPP_ENGINE_TRANSFORMCOMPONENT_H
