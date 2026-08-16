#include "Prefab.h"

#include "assets/AssetManager.h"
#include "components/impl/ModelRendererComponent.h"
#include "components/impl/LuaScriptComponent.h"
#include "components/impl/PrefabInstanceComponent.h"
#include "components/impl/TransformComponent.h"
#include "core/EngineData.h"
#include "core/Scene.h"
#include "core/SceneManager.h"
#include "utils/Logger.h"

#include <algorithm>
#include <functional>
#include <map>
#include <unordered_map>
#include <unordered_set>

namespace Engine {
	namespace {
		int HexDigit(char c)
		{
			if (c >= '0' && c <= '9') return c - '0';
			if (c >= 'a' && c <= 'f') return c - 'a' + 10;
			if (c >= 'A' && c <= 'F') return c - 'A' + 10;
			return 0;
		}

		char ToHex(int v)
		{
			return "0123456789abcdef"[v & 15];
		}

		// 128-bit hex add, wraps modulo 2^128. GUIDs are 32 lowercase hex digits.
		std::string AddGuidOffset(const std::string& guid, const std::string& offset)
		{
			if (guid.size() != 32 || offset.size() != 32) {
				return guid;
			}
			std::string out(32, '0');
			int         carry = 0;
			for (int i = 31; i >= 0; --i) {
				const int sum = HexDigit(guid[i]) + HexDigit(offset[i]) + carry;
				out[static_cast<size_t>(i)] = ToHex(sum);
				carry                       = sum >> 4;
			}
			return out;
		}

		std::string RandomGuidOffset()
		{
			return AssetManager::GenerateGUID();
		}

		bool IsInner(const std::unordered_set<std::string>& inner, const EntityHandle& handle)
		{
			return handle.IsValid() && inner.count(handle.GetID()) > 0;
		}

		void RemapHandle(EntityHandle& handle, const std::unordered_set<std::string>& inner, const std::string& offset)
		{
			if (!IsInner(inner, handle)) {
				return;
			}
			handle = EntityHandle(AddGuidOffset(handle.GetID(), offset));
		}

		void RemapScriptVariable(ScriptVariable& value, const std::unordered_set<std::string>& inner, const std::string& offset)
		{
			std::visit(
			    [&](auto& held) {
				    using T = std::decay_t<decltype(held)>;
				    if constexpr (std::is_same_v<T, EntityHandle>) {
					    RemapHandle(held, inner, offset);
				    }
				    else if constexpr (std::is_same_v<T, EntityHandleList>) {
					    for (auto& h : held) {
						    RemapHandle(h, inner, offset);
					    }
				    }
			    },
			    value);
		}

		void CollectSubtree(Entity entity, std::vector<Entity>& out)
		{
			if (!entity || !entity.IsValid()) {
				return;
			}
			out.push_back(entity);
			if (!entity.HasComponent<Components::EntityMetadata>()) {
				return;
			}
			for (const auto& childHandle : entity.GetComponent<Components::EntityMetadata>().children) {
				Entity child = entity.m_scene->Get(childHandle);
				CollectSubtree(child, out);
			}
		}

		void CaptureEntity(Entity entity, const std::unordered_set<std::string>& inner, bool isRoot, SerializedEntity& se)
		{
			if (entity.HasComponent<Components::LuaScript>()) {
				entity.GetComponent<Components::LuaScript>().SyncFromLua();
			}

			se.meta = entity.GetComponent<Components::EntityMetadata>();
			if (!IsInner(inner, se.meta.parentEntity)) {
				se.meta.parentEntity = EntityHandle();
			}
			se.meta.children.erase(
			    std::remove_if(se.meta.children.begin(), se.meta.children.end(), [&](const EntityHandle& h) { return !IsInner(inner, h); }),
			    se.meta.children.end());
			se.meta.toBeDestroyedNextUpdate = false;

#define X(type, name, fancy)                                                                                                                                                                                                                   \
			if (entity.HasComponent<type>()) {                                                                                                                                                                                                 \
				se.name = entity.GetComponent<type>();                                                                                                                                                                                         \
			}
			COMPONENT_LIST
#undef X

			// PrefabInstance is a live link to the source asset, not part of the template.
			se.PrefabInstance.reset();

			if (isRoot && se.Transform.has_value() && entity.HasComponent<Components::Transform>()) {
				auto& live = entity.GetComponent<Components::Transform>();
				auto& saved = *se.Transform;
				saved.SetLocalPosition(live.GetWorldPosition());
				saved.SetLocalRotation(live.GetWorldRotation());
				saved.SetLocalScale(live.GetWorldScale());
			}
		}

		void RemapSerializedEntity(SerializedEntity& se, const std::unordered_set<std::string>& inner, const std::string& offset)
		{
			se.meta.guid = AddGuidOffset(se.meta.guid, offset);
			RemapHandle(se.meta.parentEntity, inner, offset);
			for (auto& child : se.meta.children) {
				RemapHandle(child, inner, offset);
			}
			if (se.LuaScript.has_value()) {
				for (auto& [key, value] : se.LuaScript->cppVariables) {
					RemapScriptVariable(value, inner, offset);
				}
			}
		}

		bool OffsetCollides(const std::vector<SerializedEntity>& entities, const std::string& offset, Scene* scene)
		{
			for (const auto& se : entities) {
				const EntityHandle mapped(AddGuidOffset(se.meta.guid, offset));
				if (scene->m_entityMap.count(mapped)) {
					return true;
				}
			}
			return false;
		}
	} // namespace

	bool Prefab::CaptureFromEntity(Entity root, Prefab& out)
	{
		out = Prefab();
		if (!root || !root.IsValid() || !root.HasComponent<Components::EntityMetadata>()) {
			GetDefaultLogger()->error("[Prefab] CaptureFromEntity: invalid root");
			return false;
		}

		std::vector<Entity> subtree;
		CollectSubtree(root, subtree);
		if (subtree.empty()) {
			return false;
		}

		std::unordered_set<std::string> inner;
		inner.reserve(subtree.size());
		for (Entity& e : subtree) {
			if (e.HasComponent<Components::EntityMetadata>()) {
				inner.insert(e.GetComponent<Components::EntityMetadata>().guid);
			}
		}

		out.m_name    = root.GetName();
		out.rootGuid  = root.GetComponent<Components::EntityMetadata>().guid;
		out.entities.reserve(subtree.size());
		for (Entity& e : subtree) {
			SerializedEntity se;
			const bool       isRoot = e.GetComponent<Components::EntityMetadata>().guid == out.rootGuid;
			CaptureEntity(e, inner, isRoot, se);
			out.entities.push_back(std::move(se));
		}
		return true;
	}

	Entity Prefab::Instantiate(Scene* scene, const EntityHandle& parent) const
	{
		if (!scene || entities.empty() || rootGuid.empty()) {
			GetDefaultLogger()->error("[Prefab] Instantiate: empty prefab '{}'", m_name);
			return {};
		}

		std::unordered_set<std::string> inner;
		inner.reserve(entities.size());
		for (const auto& se : entities) {
			if (!se.meta.guid.empty()) {
				inner.insert(se.meta.guid);
			}
		}

		std::string offset;
		for (int attempt = 0; attempt < 8; ++attempt) {
			offset = RandomGuidOffset();
			if (!OffsetCollides(entities, offset, scene)) {
				break;
			}
		}

		std::vector<SerializedEntity> spawned = entities;
		for (auto& se : spawned) {
			RemapSerializedEntity(se, inner, offset);
		}
		const std::string mappedRoot = AddGuidOffset(rootGuid, offset);

		std::map<EntityHandle, Entity> created;
		Entity                         rootEntity;

		for (auto& se : spawned) {
			entt::entity handle = scene->GetRegistry()->create();
			scene->GetRegistry()->emplace<Components::EntityMetadata>(handle, se.meta);
			Entity entity(handle, scene);
			created[EntityHandle(se.meta.guid)] = entity;
			scene->m_entityList.push_back(entity);
			scene->m_entityMap[EntityHandle(se.meta.guid)] = entity;
			if (se.meta.guid == mappedRoot) {
				rootEntity = entity;
			}

#define X(type, name, fancy)                                                                                                                                                                                                                   \
			if (se.name.has_value()) {                                                                                                                                                                                                         \
				entity.AddComponent<type>(se.name.value());                                                                                                                                                                                    \
			}
			COMPONENT_LIST
#undef X
		}

		if (!rootEntity) {
			GetDefaultLogger()->error("[Prefab] Instantiate: remapped root missing in '{}'", m_name);
			return {};
		}

		GetSceneManager().UpdateTransforms();

		if (parent.IsValid()) {
			rootEntity.SetParent(parent);
		}

		return rootEntity;
	}

	std::vector<PreviewDrawItem> Prefab::CollectPreviewDraws() const
	{
		std::unordered_map<std::string, const SerializedEntity*> byGuid;
		byGuid.reserve(entities.size());
		for (const auto& se : entities) {
			if (!se.meta.guid.empty()) {
				byGuid[se.meta.guid] = &se;
			}
		}

		std::unordered_map<std::string, glm::mat4> world;
		world.reserve(entities.size());

		std::function<glm::mat4(const std::string&)> computeWorld = [&](const std::string& guid) -> glm::mat4 {
			auto cached = world.find(guid);
			if (cached != world.end()) {
				return cached->second;
			}
			auto it = byGuid.find(guid);
			if (it == byGuid.end()) {
				return glm::mat4(1.0f);
			}
			const SerializedEntity& se = *it->second;
			glm::mat4               local(1.0f);
			if (se.Transform.has_value()) {
				local = se.Transform->GetLocalMatrix();
			}
			glm::mat4 parentWorld(1.0f);
			const std::string& parentId = se.meta.parentEntity.GetID();
			if (!parentId.empty() && byGuid.count(parentId) && parentId != guid) {
				parentWorld = computeWorld(parentId);
			}
			const glm::mat4 w = parentWorld * local;
			world[guid]       = w;
			return w;
		};

		std::vector<PreviewDrawItem> items;
		for (const auto& se : entities) {
			if (!se.ModelRenderer.has_value() || !se.ModelRenderer->visible) {
				continue;
			}
			const ModelHandle& handle = se.ModelRenderer->model;
			if (!handle.IsValid()) {
				continue;
			}
			Rendering::Model* model = GetAssetManager().Get(handle);
			if (!model) {
				continue;
			}
			PreviewDrawItem item;
			item.model              = model;
			item.world              = computeWorld(se.meta.guid);
			item.materialOverrides  = se.ModelRenderer->materialOverrides;
			items.push_back(std::move(item));
		}
		return items;
	}

	Entity InstantiatePrefab(const PrefabHandle& handle, const EntityHandle& parent)
	{
		Prefab* prefab = GetAssetManager().Get(handle);
		if (!prefab) {
			GetDefaultLogger()->error("[Prefab] InstantiatePrefab: invalid handle {}", handle.GetID());
			return {};
		}
		Entity root = prefab->Instantiate(GetCurrentScene(), parent);
		if (root && root.IsValid()) {
			if (root.HasComponent<Components::PrefabInstance>()) {
				root.GetComponent<Components::PrefabInstance>().prefab = handle;
			}
			else {
				root.AddComponent<Components::PrefabInstance>(handle);
			}
		}
		return root;
	}
} // namespace Engine
