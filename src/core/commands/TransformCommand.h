#pragma once

#include "core/EditorCommand.h"
#include "core/EntityHandle.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Engine {

	/// Command for undoing/redoing transform changes (from ImGuizmo or inspector)
	class TransformCommand : public EditorCommand {
	  public:
		TransformCommand(const EntityHandle& entityHandle,
		                 const glm::vec3&     oldPosition,
		                 const glm::quat&     oldRotation,
		                 const glm::vec3&     oldScale,
		                 const glm::vec3&     newPosition,
		                 const glm::quat&     newRotation,
		                 const glm::vec3&     newScale);

		void        Execute() override;
		void        Undo() override;
		std::string GetDescription() const override;

	  private:
		EntityHandle m_entityHandle;
		glm::vec3    m_oldPosition;
		glm::quat    m_oldRotation;
		glm::vec3    m_oldScale;
		glm::vec3    m_newPosition;
		glm::quat    m_newRotation;
		glm::vec3    m_newScale;

		void ApplyTransform(const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale);
	};

} // namespace Engine
