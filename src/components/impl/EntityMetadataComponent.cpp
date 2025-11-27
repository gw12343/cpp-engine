//
// Created by gabe on 6/24/25.
//
#include "components/Components.h"
#include "imgui.h"
#include "EntityMetadataComponent.h"
#include "core/Entity.h"
#include "rendering/ui/InspectorUI.h"

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