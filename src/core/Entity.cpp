#include "Entity.h"

#include "Engine.h"
#include "components/Components.h"
#include "EngineData.h"
#include "components/impl/EntityMetadataComponent.h"

namespace Engine {

	Entity Entity::Create(const std::string& name, Scene* scene)
	{
		entt::entity entityHandle = GetCurrentSceneRegistry().create();
		Entity       entity(entityHandle, scene);

		// Add default components
		entity.AddComponent<Components::EntityMetadata>(name);

		return entity;
	}

	void Entity::Destroy(Entity entity)
	{
		if (entity) {
			GetCurrentSceneRegistry().destroy(entity.GetHandle());
		}
	}

	// Implementation of Entity metadata helpers
	const std::string& Entity::GetName() const
	{
		auto& registry = GetCurrentSceneRegistry();
		return registry.get<Components::EntityMetadata>(m_handle).name;
	}

	void Entity::SetName(const std::string& name)
	{
		auto& registry                                          = GetCurrentSceneRegistry();
		registry.get<Components::EntityMetadata>(m_handle).name = name;
	}

	[[maybe_unused]] const std::string& Entity::GetTag() const
	{
		auto& registry = GetCurrentSceneRegistry();
		return registry.get<Components::EntityMetadata>(m_handle).tag;
	}

	[[maybe_unused]] void Entity::SetTag(const std::string& tag)
	{
		auto& registry                                         = GetCurrentSceneRegistry();
		registry.get<Components::EntityMetadata>(m_handle).tag = tag;
	}

	bool Entity::IsActive() const
	{
		auto& registry = GetCurrentSceneRegistry();
		return registry.get<Components::EntityMetadata>(m_handle).active;
	}

	[[maybe_unused]] void Entity::SetActive(bool active)
	{
		auto& registry                                            = GetCurrentSceneRegistry();
		registry.get<Components::EntityMetadata>(m_handle).active = active;
	}

} // namespace Engine
