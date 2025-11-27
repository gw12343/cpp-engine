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
		auto reg = m_scene->GetRegistry();

		if (reg->valid(GetENTTHandle())) {
			if (HasComponent<Components::EntityMetadata>()) {
				auto& em                   = GetComponent<Components::EntityMetadata>();
				em.toBeDestroyedNextUpdate = true; // mark for destruction


				// remove as child of parent
				EntityHandle parentHandle = em.parentEntity;
				if (parentHandle.IsValid()) {
					Entity parent = GetCurrentScene()->Get(parentHandle);
					parent.RemoveChild(GetEntityHandle());
				}

				// remove as parent of children
				for (auto& c : em.children) {
					Entity e = GetCurrentScene()->Get(c);
					e.SetParent(EntityHandle()); // unparent
				}
			}
		}
	}
	void Entity::Destroy()
	{
		auto reg = m_scene->GetRegistry();

		if (reg->valid(GetENTTHandle())) {
			if (HasComponent<Components::EntityMetadata>()) {
				auto&        em           = GetComponent<Components::EntityMetadata>();
				EntityHandle parentHandle = em.parentEntity;
				if (parentHandle.IsValid()) {
					Entity parent = GetCurrentScene()->Get(parentHandle);
					parent.RemoveChild(GetEntityHandle());
				}

				em.OnRemoved(*this);
				for (auto& c : em.children) {
					Entity e = GetCurrentScene()->Get(c);
					e.SetParent(EntityHandle()); // unparent
					e.Destroy();
				}
			}

#define X(type, name, fancy)                                                                                                                                                                                                                   \
	if (HasComponent<type>()) {                                                                                                                                                                                                                \
		GetComponent<type>().OnRemoved(*this);                                                                                                                                                                                                 \
	}
			COMPONENT_LIST
#undef X

			reg->destroy(GetENTTHandle());
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
		EntityHandle childHandle    = EntityHandle(childHierarchy.guid);


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
		Entity par   = GetCurrentScene()->Get(newParent);
		Entity child = GetCurrentScene()->Get(childHandle);

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

		auto& childTr = registry.get<Components::Transform>(m_handle);

		SetWorldTransform(childTr.GetWorldPosition(), childTr.GetWorldRotation(), childTr.GetWorldScale());
		// --- 4. Optional: maintain world transform consistency ---
		//		if (registry.any_of<Components::Transform>(m_handle)) {
		//			auto& childTr = registry.get<Components::Transform>(m_handle);
		//
		//			if (par.HasComponent<Components::Transform>()) {
		//				auto& parentTr = par.GetComponent<Components::Transform>();
		//
		//				// Convert child's world transform into new local space
		//				glm::mat4 parentInv = glm::inverse(parentTr.worldMatrix);
		//				glm::mat4 localMat  = parentInv * childTr.worldMatrix;
		//
		//				glm::vec3 skew;
		//				glm::vec4 persp;
		//				glm::quat localRot;
		//				glm::vec3 localTrans, localScale;
		//				glm::decompose(localMat, localScale, localRot, localTrans, skew, persp);
		//
		//				childTr.localPosition = localTrans;
		//				childTr.localRotation = localRot;
		//				childTr.localScale    = localScale;
		//			}
		//			else {
		//				// Convert child's world transform into new local space
		//				glm::mat4 parentInv = glm::inverse(glm::mat4(1.0));
		//				glm::mat4 localMat  = parentInv * childTr.worldMatrix;
		//
		//				glm::vec3 skew;
		//				glm::vec4 persp;
		//				glm::quat localRot;
		//				glm::vec3 localTrans, localScale;
		//				glm::decompose(localMat, localScale, localRot, localTrans, skew, persp);
		//
		//				childTr.localPosition = localTrans;
		//				childTr.localRotation = localRot;
		//				childTr.localScale    = localScale;
		//			}
		//		}
	}


	void Entity::SetWorldTransform(glm::vec3 worldPosition, glm::quat worldRotation, glm::vec3 worldScale)
	{
		if (!HasComponent<Components::Transform>()) {
			// todo warn
			return;
		}

		auto& hr = GetComponent<Components::EntityMetadata>();
		auto& tr = GetComponent<Components::Transform>();

		worldPosition = worldPosition;
		worldRotation = worldRotation;


		if (!hr.parentEntity.IsValid()) {
			// Root player: local == world
			tr.SetLocalPosition(worldPosition);
			tr.SetLocalRotation(worldRotation);
		}
		else {
			// Child player: world -> local
			auto parentEntity = GetCurrentScene()->Get(hr.parentEntity);
			if (parentEntity && parentEntity.HasComponent<Engine::Components::Transform>()) {
				auto&     parentTr  = parentEntity.GetComponent<Engine::Components::Transform>();
				glm::mat4 parentInv = glm::inverse(parentTr.GetWorldMatrix());

				glm::mat4 worldMatrix = glm::translate(glm::mat4(1.0f), worldPosition) * glm::toMat4(worldRotation) * glm::scale(glm::mat4(1.0f), tr.GetWorldScale());

				glm::mat4 localMatrix = parentInv * worldMatrix;

				tr.SetLocalPosition(glm::vec3(localMatrix[3]));
				tr.SetLocalRotation(glm::quat_cast(localMatrix));
				tr.SetLocalScale(glm::vec3(glm::length(glm::vec3(localMatrix[0])), glm::length(glm::vec3(localMatrix[1])), glm::length(glm::vec3(localMatrix[2]))));
			}
		}


		// Update world matrix for consistency (optional if Scene::UpdateTransforms runs afterward)
		tr.SetWorldMatrix(glm::translate(glm::mat4(1.0f), tr.GetWorldPosition()) * glm::toMat4(tr.GetWorldRotation()) * glm::scale(glm::mat4(1.0f), tr.GetWorldScale()));
	}
	std::vector<EntityHandle> Entity::GetChildren()
	{
		auto& meta = GetComponent<Components::EntityMetadata>();
		return meta.GetChildren();
	}
	EntityHandle Entity::GetEntityHandle()
	{
		return EntityHandle(GetComponent<Components::EntityMetadata>().guid);
	}
	void Entity::RemoveChild(const EntityHandle& handle)
	{
		auto& meta = GetComponent<Components::EntityMetadata>();
		auto& v    = meta.children;

		auto it = std::find(v.begin(), v.end(), handle);
		if (it != v.end()) v.erase(it);
	}


} // namespace Engine
