//
// Created by gabe on 7/1/25.
//

#ifndef CPP_ENGINE_TERRAINRENDERERCOMPONENT_H
#define CPP_ENGINE_TERRAINRENDERERCOMPONENT_H

#include "components/Components.h"
#include "terrain/TerrainManager.h"

#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/array.hpp>

namespace Engine::Components {
	class TerrainRenderer : public Component {
	  public:
		AssetHandle<Terrain::TerrainTile> terrainTile;
		bool                              visible = true;

		TerrainRenderer() = default;

		template <class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("visible", visible), cereal::make_nvp("terrainTile", terrainTile) // serialize only the GUID
			);
		}

		explicit TerrainRenderer(const AssetHandle<Terrain::TerrainTile>& tile) : terrainTile(tile) {}

		static void AddBindings();

		void OnAdded(Entity& entity) override;
		void OnRemoved(Entity& entity) override;
		void RenderInspector(Entity& entity) override;
	};
} // namespace Engine::Components


#endif // CPP_ENGINE_TERRAINRENDERERCOMPONENT_H
