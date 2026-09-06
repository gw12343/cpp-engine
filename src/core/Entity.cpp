#include "Entity.h"

#include "Engine.h"
#include "EntityHandle.h"
#include "components/impl/EntityMetadataComponent.h"
#include "components/AllComponents.h"

#include "glm/gtx/matrix_decompose.inl"

#include <algorithm>


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
					if (e.IsValid()) {
						e.SetParent(EntityHandle());
					}
				}
			}
		}
	}


	void Entity::Destroy()
	{
		if (!IsValid()) {
			return;
		}
		auto reg = m_scene->GetRegistry();

		std::vector<EntityHandle> children;
		EntityHandle              parentHandle;
		if (HasComponent<Components::EntityMetadata>()) {
			auto& em     = GetComponent<Components::EntityMetadata>();
			children     = em.children;
			parentHandle = em.parentEntity;
			em.children.clear();
			em.parentEntity = EntityHandle();
		}

		for (const auto& childHandle : children) {
			Entity child = m_scene->Get(childHandle);
			if (child.IsValid()) {
				child.Destroy();
			}
		}

		// A cycle or overlapping Destroy can kill this entity while we were in children.
		if (!IsValid()) {
			return;
		}

		if (parentHandle.IsValid()) {
			Entity parent = m_scene->Get(parentHandle);
			if (parent.IsValid()) {
				parent.RemoveChild(GetEntityHandle());
			}
		}

		if (HasComponent<Components::EntityMetadata>()) {
			GetComponent<Components::EntityMetadata>().OnRemoved(*this);
		}

#define X(type, name, fancy)                                                                                                                                                                                                                   \
		if (HasComponent<type>()) {                                                                                                                                                                                                            \
			GetComponent<type>().OnRemoved(*this);                                                                                                                                                                                             \
		}
		COMPONENT_LIST
#undef X

		if (IsValid()) {
			reg->destroy(GetENTTHandle());
		}
	}

	// Implementation of Entity metadata helpers
	const std::string& Entity::GetName() const
	{
		static const std::string kEmpty;
		if (const auto* meta = TryGetComponent<Components::EntityMetadata>()) {
			return meta->name;
		}
		return kEmpty;
	}

	void Entity::SetName(const std::string& name)
	{
		if (auto* meta = TryGetComponent<Components::EntityMetadata>()) {
			meta->name = name;
		}
	}

	[[maybe_unused]] const std::string& Entity::GetTag() const
	{
		static const std::string kEmpty;
		if (const auto* meta = TryGetComponent<Components::EntityMetadata>()) {
			return meta->tag;
		}
		return kEmpty;
	}

	[[maybe_unused]] void Entity::SetTag(const std::string& tag)
	{
		if (auto* meta = TryGetComponent<Components::EntityMetadata>()) {
			meta->tag = tag;
		}
	}

	bool Entity::IsActive() const
	{
		if (const auto* meta = TryGetComponent<Components::EntityMetadata>()) {
			return meta->active;
		}
		return false;
	}

	[[maybe_unused]] void Entity::SetActive(bool active)
	{
		if (auto* meta = TryGetComponent<Components::EntityMetadata>()) {
			meta->active = active;
		}
	}
	bool Entity::IsValid() const
	{
		if (m_handle == entt::null) return false;
		if (m_scene == nullptr) return false;
		auto reg = m_scene->GetRegistry();
		if (!reg) return false;
		return reg->valid(m_handle);
	}

	void Entity::SetParent(const EntityHandle& newParent)
	{
		if (!IsValid() || !HasComponent<Components::EntityMetadata>()) {
			return;
		}

		auto&        childHierarchy = GetComponent<Components::EntityMetadata>();
		EntityHandle childHandle    = EntityHandle(childHierarchy.guid);

		if (newParent.IsValid() && newParent == childHandle) {
			return;
		}
		if (childHierarchy.parentEntity == newParent) {
			return;
		}

		if (childHierarchy.parentEntity.IsValid()) {
			Entity childHParent = m_scene->Get(childHierarchy.parentEntity);
			if (childHParent.IsValid()) {
				childHParent.RemoveChild(childHandle);
			}
		}

		if (!newParent.IsValid()) {
			childHierarchy.parentEntity = EntityHandle();
			return;
		}

		Entity par = m_scene->Get(newParent);
		if (!par.IsValid() || !par.HasComponent<Components::EntityMetadata>()) {
			childHierarchy.parentEntity = EntityHandle();
			return;
		}

		childHierarchy.parentEntity = newParent;
		auto& newChildren           = par.GetComponent<Components::EntityMetadata>().children;
		if (std::find(newChildren.begin(), newChildren.end(), childHandle) == newChildren.end()) {
			newChildren.push_back(childHandle);
		}

		if (HasComponent<Components::Transform>()) {
			auto& childTr = GetComponent<Components::Transform>();
			SetWorldTransform(childTr.GetWorldPosition(), childTr.GetWorldRotation(), childTr.GetWorldScale());
		}
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

		if (!hr.parentEntity.IsValid()) {
			tr.SetLocalPosition(worldPosition);
			tr.SetLocalRotation(worldRotation);
			tr.SetLocalScale(worldScale);
		}
		else {
			auto parentEntity = m_scene->Get(hr.parentEntity);
			if (parentEntity.IsValid() && parentEntity.HasComponent<Engine::Components::Transform>()) {
				auto& parentTr = parentEntity.GetComponent<Engine::Components::Transform>();
				tr.SetLocalFromWorld(parentTr.GetWorldMatrix(), worldPosition, worldRotation, worldScale);
			}
			else {
				tr.SetLocalPosition(worldPosition);
				tr.SetLocalRotation(worldRotation);
				tr.SetLocalScale(worldScale);
			}
		}

		tr.SetWorldFromMatrix(Components::Transform::ComposeTRS(worldPosition, worldRotation, worldScale));
	}
	std::vector<EntityHandle> Entity::GetChildren()
	{
		if (auto* meta = TryGetComponent<Components::EntityMetadata>()) {
			return meta->GetChildren();
		}
		return {};
	}
	EntityHandle Entity::GetEntityHandle()
	{
		if (auto* meta = TryGetComponent<Components::EntityMetadata>()) {
			return EntityHandle(meta->guid);
		}
		return {};
	}
	void Entity::RemoveChild(const EntityHandle& handle)
	{
		if (!IsValid() || !HasComponent<Components::EntityMetadata>()) {
			return;
		}
		auto& meta = GetComponent<Components::EntityMetadata>();
		auto& v    = meta.children;

		auto it = std::find(v.begin(), v.end(), handle);
		if (it != v.end()) v.erase(it);
	}


} // namespace Engine
