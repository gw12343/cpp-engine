#include "Entity.h"
#include "Engine.h"
#include "components/Components.h"

namespace Engine {

Entity Entity::Create(GEngine* engine, const std::string& name)
{
    entt::entity entityHandle = engine->GetRegistry().create();
    Entity entity(entityHandle, engine);

    // Add default components
    entity.AddComponent<Components::EntityMetadata>(name);

    return entity;
}

void Entity::Destroy(Entity entity)
{
    if (entity)
    {
        entity.GetEngine()->GetRegistry().destroy(entity.GetHandle());
    }
}

// Implementation of Entity metadata helpers
const std::string& Entity::GetName() const {
    auto& registry = m_engine->GetRegistry();
    return registry.get<Components::EntityMetadata>(m_handle).name;
}

void Entity::SetName(const std::string& name) {
    auto& registry = m_engine->GetRegistry();
    registry.get<Components::EntityMetadata>(m_handle).name = name;
}

const std::string& Entity::GetTag() const {
    auto& registry = m_engine->GetRegistry();
    return registry.get<Components::EntityMetadata>(m_handle).tag;
}

void Entity::SetTag(const std::string& tag) {
    auto& registry = m_engine->GetRegistry();
    registry.get<Components::EntityMetadata>(m_handle).tag = tag;
}

bool Entity::IsActive() const {
    auto& registry = m_engine->GetRegistry();
    return registry.get<Components::EntityMetadata>(m_handle).active;
}

void Entity::SetActive(bool active) {
    auto& registry = m_engine->GetRegistry();
    registry.get<Components::EntityMetadata>(m_handle).active = active;
}

}
