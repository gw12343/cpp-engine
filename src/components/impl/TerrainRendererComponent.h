//
// Created by gabe on 7/1/25.
//

#ifndef CPP_ENGINE_TERRAINRENDERERCOMPONENT_H
#define CPP_ENGINE_TERRAINRENDERERCOMPONENT_H

#include "assets/AssetHandle.h"
#include "components/Components.h"

#include <cereal/cereal.hpp>

namespace Engine::Components {
	class TerrainRenderer : public Component {
	  public:
		TerrainHandle terrainTile;
		bool          visible = true;

		TerrainRenderer() = default;

		template <class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("visible", visible), cereal::make_nvp("terrainTile", terrainTile));
		}

		explicit TerrainRenderer(const TerrainHandle& tile) : terrainTile(tile) {}

		static void AddBindings();

		void OnAdded(Entity& entity) override;
		void OnRemoved(Entity& entity) override;
		void RenderInspector(Entity& entity) override;
	};
} // namespace Engine::Components

#endif // CPP_ENGINE_TERRAINRENDERERCOMPONENT_H
