#pragma once

#include "assets/SerializedEntity.h"
#include "core/Entity.h"
#include "core/EntityHandle.h"
#include "rendering/ui/PreviewDrawItem.h"

#include <string>
#include <vector>

namespace Engine {
	class Scene;

	class Prefab {
	  public:
		std::string                   m_name;
		std::string                   rootGuid;
		std::vector<SerializedEntity> entities;

		const std::string& GetName() const { return m_name; }
		void               SetName(const std::string& name) { m_name = name; }

		static bool CaptureFromEntity(Entity root, Prefab& out);

		Entity Instantiate(Scene* scene, const EntityHandle& parent = EntityHandle()) const;

		std::vector<PreviewDrawItem> CollectPreviewDraws() const;
	};

	Entity InstantiatePrefab(const PrefabHandle& handle, const EntityHandle& parent = EntityHandle());
} // namespace Engine
