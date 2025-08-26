//
// Created by gabe on 6/24/25.
//
#include "components/Components.h"
#include "imgui.h"
#include "EntityMetadataComponent.h"
#include "core/Entity.h"

namespace Engine::Components {
	void EntityMetadata::OnRemoved(Entity& entity)
	{
	}

	static std::string GenerateGUID()
	{
		std::stringstream               ss;
		std::random_device              rd;
		std::mt19937                    gen(rd());
		std::uniform_int_distribution<> dis(0, 15);

		for (int i = 0; i < 32; ++i)
			ss << std::hex << dis(gen);
		return ss.str();
	}


	void EntityMetadata::OnAdded(Entity& entity)
	{
		if (guid.empty()) {
			guid = GenerateGUID();
		}
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
		ImGui::Text("id: %d", entity.GetHandle());
	}

	EntityMetadata::EntityMetadata()
	{
		if (guid.empty()) {
			guid = GenerateGUID();
		}
	}
	EntityMetadata::EntityMetadata(std::string name) : name(std::move(name))
	{
		if (guid.empty()) {
			guid = GenerateGUID();
		}
	}
	EntityMetadata::EntityMetadata(std::string name, std::string tag) : name(std::move(name)), tag(std::move(tag))
	{
		if (guid.empty()) {
			guid = GenerateGUID();
		}
	}
} // namespace Engine::Components