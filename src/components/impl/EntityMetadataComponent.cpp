//
// Created by gabe on 6/24/25.
//
#include "components/Components.h"
#include "imgui.h"
#include "EntityMetadataComponent.h"


namespace Engine::Components {
	void EntityMetadata::OnRemoved(Entity& entity)
	{
	}
	void EntityMetadata::OnAdded(Entity& entity)
	{
	}

	void EntityMetadata::RenderInspector(Entity& entity)
	{
		char nameBuffer[256];
		strcpy(nameBuffer, name.c_str());
		if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
			name = nameBuffer;
		}

		char tagBuffer[256];
		strcpy(tagBuffer, tag.c_str());
		if (ImGui::InputText("Tag", tagBuffer, sizeof(tagBuffer))) {
			tag = tagBuffer;
		}

		ImGui::Checkbox("Active", &active);
	}
} // namespace Engine::Components