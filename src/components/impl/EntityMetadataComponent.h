//
// Created by gabe on 6/30/25.
//

#ifndef CPP_ENGINE_ENTITYMETADATACOMPONENT_H
#define CPP_ENGINE_ENTITYMETADATACOMPONENT_H

#include "components/Components.h"

namespace Engine::Components {
	// Basic metadata for an entity
	class EntityMetadata : public Component {
	  public:
		std::string name;
		std::string tag;
		bool        active = true;

		EntityMetadata() = default;
		explicit EntityMetadata(std::string name) : name(std::move(name)) {}
		explicit EntityMetadata(std::string name, std::string tag) : name(std::move(name)), tag(std::move(tag)) {}

		void OnAdded(Entity& entity) override;
		void RenderInspector(Entity& entity) override;
	};
} // namespace Engine::Components

#endif // CPP_ENGINE_ENTITYMETADATACOMPONENT_H
