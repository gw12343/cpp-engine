//
// Created by gabe on 7/1/25.
//

#include "TerrainRendererComponent.h"
#include "misc/cpp/imgui_stdlib.h"

namespace Engine {

	void Components::TerrainRenderer::OnRemoved(Entity& entity)
	{
	}
	void Components::TerrainRenderer::OnAdded(Entity& entity)
	{
	}

	void Components::TerrainRenderer::RenderInspector(Entity& entity)
	{
		std::string newID = terrainTile.GetID();
		if (ImGui::InputText("Terrain Tile", &newID)) {
			terrainTile = AssetHandle<Terrain::TerrainTile>(newID);
		}

		if (!terrainTile.IsValid()) {
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "Invalid terrain tile!");
		}
		else {
			auto tile = GetAssetManager().Get(terrainTile);
			if (tile != NULL) {
				ImGui::Indent();

				ImGui::Text("Name: %s", tile->name.c_str());
				ImGui::Text("Heightmap Res: %d", tile->heightRes);
				ImGui::Text("Splat Res: %d", tile->splatRes);
				ImGui::Text("Splat Layer Count: %d", tile->splatLayerCount);
				ImGui::Text("Size X: %d", tile->sizeX);
				ImGui::Text("Size Y: %d", tile->sizeY);
				ImGui::Text("Size Z: %d", tile->sizeZ);

				ImGui::Unindent();
			}
		}
	}
	void Components::TerrainRenderer::AddBindings()
	{
	}
} // namespace Engine

#include "assets/AssetManager.inl"