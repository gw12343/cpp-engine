//
// Created by gabe on 7/1/25.
//

#ifndef CPP_ENGINE_TERRAINRENDERERCOMPONENT_H
#define CPP_ENGINE_TERRAINRENDERERCOMPONENT_H

#include "components/Components.h"
#include "terrain/TerrainManager.h"


namespace Engine::Components {
	class TerrainRenderer : public Component {
	  public:
		AssetHandle<Terrain::TerrainTile> terrainTile;
		bool                              visible = true;

		TerrainRenderer() = default;

		explicit TerrainRenderer(const AssetHandle<Terrain::TerrainTile>& tile) : terrainTile(tile) {}

		static void AddBindings();

		void OnAdded(Entity& entity) override;
		void RenderInspector(Entity& entity) override;
	};
} // namespace Engine::Components


#endif // CPP_ENGINE_TERRAINRENDERERCOMPONENT_H
