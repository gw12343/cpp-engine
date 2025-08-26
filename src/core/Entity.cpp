#include "Entity.h"

#include "Engine.h"
#include "EngineData.h"
#include "components/impl/EntityMetadataComponent.h"
#include "components/AllComponents.h"

namespace Engine {

	Entity Entity::Create(const std::string& name, Scene* scene)
	{
		entt::entity entityHandle = scene->GetRegistry()->create();
		Entity       entity(entityHandle, scene);

		// Add default components
		entity.AddComponent<Components::EntityMetadata>(name);

		return entity;
	}

	void Entity::MarkForDestruction()
	{
		GetComponent<Components::EntityMetadata>().toBeDestroyedNextUpdate = true;
	}
	void Entity::Destroy()
	{
		auto reg = m_scene->GetRegistry();
		if (reg->valid(GetHandle())) {
			if (HasComponent<Components::EntityMetadata>()) {
				GetComponent<Components::EntityMetadata>().OnRemoved(*this);
			}

#define X(type, name, fancy)                                                                                                                                                                                                                   \
	if (HasComponent<type>()) {                                                                                                                                                                                                                \
		GetComponent<type>().OnRemoved(*this);                                                                                                                                                                                                 \
	}
			COMPONENT_LIST
#undef X

			reg->destroy(GetHandle());
		}
	}

	// Implementation of Entity metadata helpers
	const std::string& Entity::GetName() const
	{
		auto registry = m_scene->GetRegistry();
		return registry->get<Components::EntityMetadata>(m_handle).name;
	}

	void Entity::SetName(const std::string& name)
	{
		auto registry                                            = m_scene->GetRegistry();
		registry->get<Components::EntityMetadata>(m_handle).name = name;
	}

	[[maybe_unused]] const std::string& Entity::GetTag() const
	{
		auto registry = m_scene->GetRegistry();
		return registry->get<Components::EntityMetadata>(m_handle).tag;
	}

	[[maybe_unused]] void Entity::SetTag(const std::string& tag)
	{
		auto registry                                           = m_scene->GetRegistry();
		registry->get<Components::EntityMetadata>(m_handle).tag = tag;
	}

	bool Entity::IsActive() const
	{
		auto registry = m_scene->GetRegistry();
		return registry->get<Components::EntityMetadata>(m_handle).active;
	}

	[[maybe_unused]] void Entity::SetActive(bool active)
	{
		auto registry                                              = m_scene->GetRegistry();
		registry->get<Components::EntityMetadata>(m_handle).active = active;
	}
	bool Entity::IsValid()
	{
		if (m_handle == entt::null) return false;
		if (m_scene == nullptr) return false;
		if (!m_scene->GetRegistry()->valid(m_handle)) return false;
		return true;
	}

} // namespace Engine
