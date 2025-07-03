//
// Created by gabe on 7/1/25.
//

#include "TerrainRendererComponent.h"

namespace Engine {

	void Components::TerrainRenderer::OnAdded(Entity& entity)
	{
	}

	void Components::TerrainRenderer::RenderInspector(Entity& entity)
	{
		if (!terrainTile.IsValid()) {
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "No terrain tile!");
		}

		ImGui::Indent();

		auto tile = GetAssetManager().Get(terrainTile);
		ImGui::Text("Name: %s", tile->name.c_str());
		ImGui::Text("Heightmap Res: %d", tile->heightRes);
		ImGui::Text("Splat Res: %d", tile->splatRes);
		ImGui::Text("Splat Layer Count: %d", tile->splatLayerCount);
		ImGui::Text("Size X: %d", tile->sizeX);
		ImGui::Text("Size Y: %d", tile->sizeY);
		ImGui::Text("Size Z: %d", tile->sizeZ);


		ImGui::Unindent();
	}
} // namespace Engine