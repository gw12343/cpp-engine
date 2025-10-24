#include "Entity.h"

#include "Engine.h"
#include "EntityHandle.h"
#include "components/impl/EntityMetadataComponent.h"
#include "components/AllComponents.h"
#include "utils/Utils.h"
#include "glm/gtx/matrix_decompose.inl"


namespace Engine {

	Entity Entity::Create(const std::string& name, Scene* scene)
	{
		entt::entity entityHandle = scene->GetRegistry()->create();
		Entity       entity(entityHandle, scene);

		// Add default components
		auto& meta = entity.AddComponent<Components::EntityMetadata>(name);

		scene->m_entityList.push_back(entity);
		scene->m_entityMap[EntityHandle(meta.guid)] = entity;

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

	void Entity::SetParent(const EntityHandle& newParent)
	{
		auto& registry = GetCurrentSceneRegistry();


		auto&        childHierarchy = registry.get<Components::EntityMetadata>(m_handle);
		EntityHandle childHandle    = EntityHandle(registry.get<Components::EntityMetadata>(m_handle).guid);


		// --- 1. Remove from old parent's children list ---
		if (childHierarchy.parentEntity.IsValid()) {
			Entity childHParent = GetCurrentScene()->Get(childHierarchy.parentEntity);

			if (childHParent) {
				auto& oldParentData = childHParent.GetComponent<Components::EntityMetadata>();

				oldParentData.children.erase(std::remove(oldParentData.children.begin(), oldParentData.children.end(), childHandle), oldParentData.children.end());
			}
		}

		// parent is empty
		if (!newParent.IsValid()) {
			childHierarchy.parentEntity = EntityHandle();
			GetDefaultLogger()->info("empty parent");
			return;
		}
		Entity par = GetCurrentScene()->Get(newParent);
		// entity does not exist, just set to root
		if (!par) {
			childHierarchy.parentEntity = EntityHandle();
			GetDefaultLogger()->info("bad parent");
			return;
		}

		// --- 2. Update parent link ---
		childHierarchy.parentEntity = newParent;

		// --- 3. Add to new parent's children list ---

		auto& newParentData = par.GetComponent<Components::EntityMetadata>();
		newParentData.children.push_back(childHandle);


		// --- 4. Optional: maintain world transform consistency ---
		if (registry.any_of<Components::Transform>(m_handle)) {
			auto& childTr = registry.get<Components::Transform>(m_handle);

			if (par.HasComponent<Components::Transform>()) {
				auto& parentTr = par.GetComponent<Components::Transform>();

				// Convert child's world transform into new local space
				glm::mat4 parentInv = glm::inverse(parentTr.worldMatrix);
				glm::mat4 localMat  = parentInv * childTr.worldMatrix;

				glm::vec3 skew;
				glm::vec4 persp;
				glm::quat localRot;
				glm::vec3 localTrans, localScale;
				glm::decompose(localMat, localScale, localRot, localTrans, skew, persp);

				childTr.localPosition = localTrans;
				childTr.localRotation = localRot;
				childTr.localScale    = localScale;
			}
			else {
				// Convert child's world transform into new local space
				glm::mat4 parentInv = glm::inverse(glm::mat4(1.0));
				glm::mat4 localMat  = parentInv * childTr.worldMatrix;

				glm::vec3 skew;
				glm::vec4 persp;
				glm::quat localRot;
				glm::vec3 localTrans, localScale;
				glm::decompose(localMat, localScale, localRot, localTrans, skew, persp);

				childTr.localPosition = localTrans;
				childTr.localRotation = localRot;
				childTr.localScale    = localScale;
			}
		}
	}

} // namespace Engine
