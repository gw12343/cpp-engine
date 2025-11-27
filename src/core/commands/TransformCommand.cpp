#include "TransformCommand.h"
#include "core/Entity.h"
#include "core/EngineData.h"
#include "components/impl/TransformComponent.h"
#include "components/impl/EntityMetadataComponent.h"

namespace Engine {

	TransformCommand::TransformCommand(const EntityHandle& entityHandle,
	                                   const glm::vec3&     oldPosition,
	                                   const glm::quat&     oldRotation,
	                                   const glm::vec3&     oldScale,
	                                   const glm::vec3&     newPosition,
	                                   const glm::quat&     newRotation,
	                                   const glm::vec3&     newScale)
	    : m_entityHandle(entityHandle), m_oldPosition(oldPosition), m_oldRotation(oldRotation), m_oldScale(oldScale), m_newPosition(newPosition), m_newRotation(newRotation), m_newScale(newScale)
	{
	}

	void TransformCommand::Execute()
	{
		ApplyTransform(m_newPosition, m_newRotation, m_newScale);
	}

	void TransformCommand::Undo()
	{
		ApplyTransform(m_oldPosition, m_oldRotation, m_oldScale);
	}

	std::string TransformCommand::GetDescription() const
	{
		return "Transform Entity";
	}

	void TransformCommand::ApplyTransform(const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale)
	{
		Entity entity = GetCurrentScene()->Get(m_entityHandle);
		if (!entity.IsValid()) {
			GetDefaultLogger()->warn("TransformCommand: Entity no longer valid");
			return;
		}

		if (!entity.HasComponent<Components::Transform>()) {
			GetDefaultLogger()->warn("TransformCommand: Entity has no Transform component");
			return;
		}

		auto& transform = entity.GetComponent<Components::Transform>();
		auto& metadata  = entity.GetComponent<Components::EntityMetadata>();

		// Set local transform
		transform.SetLocalPosition(position);
		transform.SetLocalRotation(rotation);
		transform.SetLocalScale(scale);

		// Update world transform based on hierarchy
		auto parentMatrix = glm::mat4(1.0f);
		if (metadata.parentEntity.IsValid()) {
			Entity parent = GetCurrentScene()->Get(metadata.parentEntity);
			if (parent && parent.HasComponent<Components::Transform>()) {
				auto& parentTr = parent.GetComponent<Components::Transform>();
				parentMatrix   = parentTr.GetWorldMatrix();
			}
		}

		glm::mat4 localMatrix = glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(rotation) * glm::scale(glm::mat4(1.0f), scale);
		transform.SetWorldMatrix(parentMatrix * localMatrix);
		transform.SetWorldPosition(glm::vec3(transform.GetWorldMatrix()[3]));
		transform.SetWorldRotation(glm::quat_cast(transform.GetWorldMatrix()));
		transform.SetWorldScale(glm::vec3(glm::length(glm::vec3(transform.GetWorldMatrix()[0])), glm::length(glm::vec3(transform.GetWorldMatrix()[1])), glm::length(glm::vec3(transform.GetWorldMatrix()[2]))));

		// Sync with physics if necessary
		transform.SyncWithPhysics(entity);
	}

} // namespace Engine
