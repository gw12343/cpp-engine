#pragma once

#include "core/EntityHandle.h"
#include <nlohmann/json.hpp>
#include <vector>

namespace Engine {
	class Entity;

	/// Manages entity copy/paste with full hierarchy support
	class EntityClipboard {
	  public:
		/// Copy an entity and all its hierarchical children to the clipboard
		void CopyEntity(const Entity& entity);

		/// Paste the copied entity hierarchy into the specified scene
		/// Returns the handle of the root pasted entity
		EntityHandle PasteEntity(class Scene* scene);

		/// Check if the clipboard has data
		bool HasData() const { return !m_clipboardData.is_null() && !m_clipboardData.empty(); }

		/// Clear clipboard
		void Clear();

	  private:
		nlohmann::json m_clipboardData;

		void SerializeEntityHierarchy(const Entity& entity, nlohmann::json& outJson);
		EntityHandle DeserializeEntityHierarchy(const nlohmann::json& json, Scene* scene, const EntityHandle& parentHandle);
	};

} // namespace Engine
