//
// Created by gabe on 6/30/25.
//

#ifndef CPP_ENGINE_ENTITYMETADATACOMPONENT_H
#define CPP_ENGINE_ENTITYMETADATACOMPONENT_H

#include "components/Components.h"
#include <cereal/cereal.hpp>
#include <cereal/types/string.hpp>

namespace Engine::Components {
	// Basic metadata for an entity
	class EntityMetadata : public Component {
	  public:
		std::string name;
		std::string tag;
		bool        active                  = true;
		bool        toBeDestroyedNextUpdate = false;
		std::string guid;

		template <class Archive>
		void serialize(Archive& ar)
		{
			ar(CEREAL_NVP(name), CEREAL_NVP(tag), CEREAL_NVP(active), CEREAL_NVP(guid));
		}

		EntityMetadata();
		explicit EntityMetadata(std::string name);
		explicit EntityMetadata(std::string name, std::string tag);

		void OnAdded(Entity& entity) override;
		void OnRemoved(Entity& entity) override;
		void RenderInspector(Entity& entity) override;
	};
} // namespace Engine::Components

#endif // CPP_ENGINE_ENTITYMETADATACOMPONENT_H
