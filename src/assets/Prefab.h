#pragma once

#include "assets/SerializedEntity.h"
#include "core/Entity.h"
#include "core/EntityHandle.h"

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

		// Capture root + descendants. Root parent is cleared; root local TRS is
		// baked to world so an unparented instance appears at the original pose.
		static bool CaptureFromEntity(Entity root, Prefab& out);

		// Spawn into scene. Inner entity GUIDs (and EntityHandle fields that point
		// at them) are offset by one random 128-bit add-with-wrap. Handles that
		// point outside the prefab are left unchanged. Asset handles and script
		// values are copied as-is (after entity-handle remap).
		Entity Instantiate(Scene* scene, const EntityHandle& parent = EntityHandle()) const;
	};

	Entity InstantiatePrefab(const PrefabHandle& handle, const EntityHandle& parent = EntityHandle());
} // namespace Engine
