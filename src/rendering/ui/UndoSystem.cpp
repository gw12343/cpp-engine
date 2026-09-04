#include "UndoSystem.h"

#include "EditorSession.h"
#include "UIManager.h"

#include "assets/SerializedEntity.h"
#include "components/AllComponents.h"
#include "components/impl/EntityMetadataComponent.h"
#include "components/impl/LuaScriptComponent.h"
#include "components/impl/TransformComponent.h"
#include "rendering/ui/IconsFontAwesome6.h"
#include "core/EngineData.h"
#include "core/Scene.h"
#include "core/SceneManager.h"

#include <cereal/archives/json.hpp>
#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <cstring>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <utility>

#ifndef GAME_BUILD

namespace Engine::UI {
	namespace {

		template <typename T>
		void CopySerializedFields(const T& src, T& dst)
		{
			std::stringstream ss;
			{
				cereal::JSONOutputArchive out(ss);
				out(cereal::make_nvp("v", const_cast<T&>(src)));
			}
			std::stringstream inStream(ss.str());
			{
				cereal::JSONInputArchive in(inStream);
				in(cereal::make_nvp("v", dst));
			}
		}

		std::string ToJson(const SerializedEntity& se)
		{
			std::stringstream ss;
			{
				cereal::JSONOutputArchive out(ss);
				out(cereal::make_nvp("entity", const_cast<SerializedEntity&>(se)));
			}
			return ss.str();
		}

		bool SameState(const SerializedEntity& a, const SerializedEntity& b)
		{
			return ToJson(a) == ToJson(b);
		}

		template <typename T>
		std::string ComponentJson(const T& value)
		{
			std::stringstream ss;
			{
				cereal::JSONOutputArchive out(ss);
				out(cereal::make_nvp("v", const_cast<T&>(value)));
			}
			return ss.str();
		}

		template <typename T>
		bool ComponentSerializedEqual(const T& a, const T& b)
		{
			return ComponentJson(a) == ComponentJson(b);
		}

		template <typename T>
		bool LuaScriptPathChanged(const T& before, const T& after)
		{
			if constexpr (std::is_same_v<T, Components::LuaScript>) {
				return before.scriptPath != after.scriptPath;
			}
			else {
				return false;
			}
		}

		const char* PrettyComponentLabel(const char* fancy)
		{
			if (!fancy || fancy[0] == '\0') {
				return "Component";
			}
			if (static_cast<unsigned char>(fancy[0]) >= 0x80) {
				const char* space = std::strchr(fancy, ' ');
				if (space && space[1] != '\0') {
					return space + 1;
				}
			}
			return fancy;
		}

		std::string DescribeModify(const SerializedEntity& before, const SerializedEntity& after)
		{
			const bool nameCh   = before.meta.name != after.meta.name;
			const bool tagCh    = before.meta.tag != after.meta.tag;
			const bool activeCh = before.meta.active != after.meta.active;
			const bool parentCh = before.meta.parentEntity != after.meta.parentEntity;
			const int  metaBits = static_cast<int>(nameCh) + static_cast<int>(tagCh) + static_cast<int>(activeCh) +
			                     static_cast<int>(parentCh);

			int         added = 0, removed = 0, edited = 0;
			const char* addedName   = nullptr;
			const char* removedName = nullptr;
			const char* editedName  = nullptr;
			bool        luaPathCh   = false;

#define X(type, name, fancy)                                                                                                                                                                                                                   \
			{                                                                                                                                                                                                                                  \
				const bool had  = before.name.has_value();                                                                                                                                                                                     \
				const bool want = after.name.has_value();                                                                                                                                                                                      \
				if (!had && want) {                                                                                                                                                                                                            \
					++added;                                                                                                                                                                                                                   \
					addedName = PrettyComponentLabel(fancy);                                                                                                                                                                                   \
				}                                                                                                                                                                                                                              \
				else if (had && !want) {                                                                                                                                                                                                       \
					++removed;                                                                                                                                                                                                                 \
					removedName = PrettyComponentLabel(fancy);                                                                                                                                                                                 \
				}                                                                                                                                                                                                                              \
				else if (had && want && !ComponentSerializedEqual(*before.name, *after.name)) {                                                                                                                                                 \
					++edited;                                                                                                                                                                                                                  \
					editedName = PrettyComponentLabel(fancy);                                                                                                                                                                                  \
					luaPathCh  = LuaScriptPathChanged(*before.name, *after.name);                                                                                                                                                              \
				}                                                                                                                                                                                                                              \
			}
			COMPONENT_LIST
#undef X

			if (added == 0 && removed == 0 && edited == 0) {
				if (metaBits == 1 && nameCh) {
					return "Rename Entity";
				}
				if (metaBits == 1 && activeCh) {
					return "Toggle Active";
				}
				if (metaBits == 1 && parentCh) {
					return "Reparent Entity";
				}
				if (metaBits == 1 && tagCh) {
					return "Edit Tag";
				}
				return "Edit Entity";
			}
			if (added == 1 && removed == 0 && edited == 0 && metaBits == 0) {
				return std::string("Add ") + addedName;
			}
			if (removed == 1 && added == 0 && edited == 0 && metaBits == 0) {
				return std::string("Remove ") + removedName;
			}
			if (edited == 1 && added == 0 && removed == 0 && metaBits == 0) {
				if (editedName && std::strcmp(editedName, "Script") == 0) {
					return luaPathCh ? "Change Script" : "Edit Script Variables";
				}
				if (editedName && std::strcmp(editedName, "Transform") == 0) {
					return "Transform";
				}
				return std::string("Edit ") + editedName;
			}
			return "Edit Entity";
		}

		template <typename T>
		void ApplyChangedComponent(Entity entity, T& live, const T& snap)
		{
			if constexpr (std::is_same_v<T, Components::Transform>) {
				CopySerializedFields(snap, live);
			}
			else if constexpr (std::is_same_v<T, Components::LuaScript>) {
				const std::string oldPath = live.scriptPath;
				CopySerializedFields(snap, live);
				if (live.scriptPath != oldPath) {
					live.LoadScript(entity, live.scriptPath);
				}
				else {
					live.SyncToLua();
				}
			}
			else {
				entity.RemoveComponent<T>();
				entity.AddComponent<T>(snap);
			}
		}

		void CollectSubtree(Entity entity, std::vector<Entity>& out, std::unordered_set<std::string>& visited)
		{
			if (!entity.IsValid() || !entity.HasComponent<Components::EntityMetadata>()) {
				return;
			}
			const std::string& guid = entity.GetComponent<Components::EntityMetadata>().guid;
			if (!visited.insert(guid).second) {
				return;
			}
			out.push_back(entity);
			const std::vector<EntityHandle> children = entity.GetComponent<Components::EntityMetadata>().children;
			for (const auto& childHandle : children) {
				CollectSubtree(entity.m_scene->Get(childHandle), out, visited);
			}
		}

		void CompactSceneEntities()
		{
			Scene* scene = GetCurrentScene();
			if (!scene) {
				return;
			}
			auto& list = scene->m_entityList;
			list.erase(std::remove_if(list.begin(), list.end(), [](Entity& e) { return !e.IsValid(); }), list.end());
			for (auto it = scene->m_entityMap.begin(); it != scene->m_entityMap.end();) {
				if (!it->second.IsValid()) {
					it = scene->m_entityMap.erase(it);
				}
				else {
					++it;
				}
			}
		}

		std::string GuidOf(Entity entity)
		{
			if (!entity || !entity.IsValid() || !entity.HasComponent<Components::EntityMetadata>()) {
				return {};
			}
			return entity.GetComponent<Components::EntityMetadata>().guid;
		}

		void ClearSelectionIfIn(const std::unordered_set<std::string>& guids)
		{
			Entity& sel = GetUI().m_selectedEntity;
			if (!sel) {
				return;
			}
			if (!sel.IsValid()) {
				sel = Entity();
				return;
			}
			const std::string guid = GuidOf(sel);
			if (!guid.empty() && guids.count(guid)) {
				sel = Entity();
			}
		}

		void ApplyEntityState(Entity entity, const SerializedEntity& se)
		{
			if (!entity || !entity.IsValid()) {
				return;
			}

			auto& meta = entity.GetComponent<Components::EntityMetadata>();
			meta.name   = se.meta.name;
			meta.tag    = se.meta.tag;
			meta.active = se.meta.active;

			if (meta.parentEntity != se.meta.parentEntity) {
				entity.SetParent(se.meta.parentEntity);
			}

#define X(type, name, fancy)                                                                                                                                                                                                                   \
			{                                                                                                                                                                                                                                  \
				const bool want = se.name.has_value();                                                                                                                                                                                         \
				const bool have = entity.HasComponent<type>();                                                                                                                                                                                 \
				if (have && want) {                                                                                                                                                                                                            \
					auto& live = entity.GetComponent<type>();                                                                                                                                                                                  \
					if (!ComponentSerializedEqual(live, *se.name)) {                                                                                                                                                                           \
						ApplyChangedComponent<type>(entity, live, *se.name);                                                                                                                                                                   \
					}                                                                                                                                                                                                                          \
				}                                                                                                                                                                                                                              \
				else if (have && !want) {                                                                                                                                                                                                      \
					entity.RemoveComponent<type>();                                                                                                                                                                                            \
				}                                                                                                                                                                                                                              \
				else if (!have && want) {                                                                                                                                                                                                      \
					entity.AddComponent<type>(*se.name);                                                                                                                                                                                       \
				}                                                                                                                                                                                                                              \
			}
			COMPONENT_LIST
#undef X

			GetSceneManager().UpdateTransforms();
			if (entity.HasComponent<Components::Transform>()) {
				entity.GetComponent<Components::Transform>().SyncWithPhysics(entity);
			}
		}

		void RelinkExternalParents(const std::vector<SerializedEntity>& entities, const std::unordered_set<std::string>& inner)
		{
			Scene* scene = GetCurrentScene();
			if (!scene) {
				return;
			}
			for (const auto& se : entities) {
				if (!se.meta.parentEntity.IsValid()) {
					continue;
				}
				if (inner.count(se.meta.parentEntity.GetID())) {
					continue;
				}
				if (!scene->m_entityMap.count(se.meta.parentEntity)) {
					continue;
				}
				Entity parent = scene->Get(se.meta.parentEntity);
				if (!parent || !parent.IsValid() || !parent.HasComponent<Components::EntityMetadata>()) {
					continue;
				}
				auto&              children = parent.GetComponent<Components::EntityMetadata>().children;
				const EntityHandle child(se.meta.guid);
				if (std::find(children.begin(), children.end(), child) == children.end()) {
					children.push_back(child);
				}
			}
		}

		Entity RestoreEntities(const std::vector<SerializedEntity>& entities)
		{
			Scene* scene = GetCurrentScene();
			if (!scene || entities.empty()) {
				return {};
			}

			std::unordered_set<std::string> inner;
			inner.reserve(entities.size());
			for (const auto& se : entities) {
				if (!se.meta.guid.empty()) {
					inner.insert(se.meta.guid);
				}
			}

			Entity first;
			for (const auto& se : entities) {
				const EntityHandle handle(se.meta.guid);
				if (scene->m_entityMap.count(handle)) {
					Entity existing = scene->Get(handle);
					if (existing && existing.IsValid()) {
						ApplyEntityState(existing, se);
						if (!first) {
							first = existing;
						}
						continue;
					}
				}

				const entt::entity id = scene->GetRegistry()->create();
				scene->GetRegistry()->emplace<Components::EntityMetadata>(id, se.meta);
				Entity entity(id, scene);
				scene->m_entityList.push_back(entity);
				scene->m_entityMap[handle] = entity;
				if (!first) {
					first = entity;
				}

#define X(type, name, fancy)                                                                                                                                                                                                                   \
				if (se.name.has_value()) {                                                                                                                                                                                                     \
					entity.AddComponent<type>(*se.name);                                                                                                                                                                                       \
				}
				COMPONENT_LIST
#undef X
			}

			RelinkExternalParents(entities, inner);
			GetSceneManager().UpdateTransforms();
			return first;
		}

		void DestroySnapshot(const std::vector<SerializedEntity>& entities)
		{
			Scene* scene = GetCurrentScene();
			if (!scene || entities.empty()) {
				return;
			}

			std::unordered_set<std::string> inner;
			for (const auto& se : entities) {
				inner.insert(se.meta.guid);
			}

			for (const auto& se : entities) {
				const bool childOfCaptured = se.meta.parentEntity.IsValid() && inner.count(se.meta.parentEntity.GetID()) > 0;
				if (childOfCaptured) {
					continue;
				}
				const EntityHandle handle(se.meta.guid);
				if (!scene->m_entityMap.count(handle)) {
					continue;
				}
				Entity entity = scene->Get(handle);
				if (entity && entity.IsValid()) {
					entity.Destroy();
				}
			}
			CompactSceneEntities();
		}

		std::unordered_set<std::string> GuidsOf(const std::vector<SerializedEntity>& entities)
		{
			std::unordered_set<std::string> guids;
			for (const auto& se : entities) {
				guids.insert(se.meta.guid);
			}
			return guids;
		}

		class ModifyEntityCommand final : public IUndoCommand {
		  public:
			ModifyEntityCommand(std::string name, SerializedEntity before, SerializedEntity after)
			    : m_name(std::move(name)), m_before(std::move(before)), m_after(std::move(after))
			{
			}

			void Undo() override
			{
				ApplyGuid(m_before);
			}

			void Redo() override
			{
				ApplyGuid(m_after);
			}

			const char* Name() const override { return m_name.c_str(); }

		  private:
			static void ApplyGuid(const SerializedEntity& se)
			{
				Scene* scene = GetCurrentScene();
				if (!scene) {
					return;
				}
				const EntityHandle handle(se.meta.guid);
				if (!scene->m_entityMap.count(handle)) {
					return;
				}
				Entity entity = scene->Get(handle);
				if (entity && entity.IsValid()) {
					ApplyEntityState(entity, se);
				}
			}

			std::string      m_name;
			SerializedEntity m_before;
			SerializedEntity m_after;
		};

		class SpawnEntitiesCommand final : public IUndoCommand {
		  public:
			SpawnEntitiesCommand(std::string name, std::vector<SerializedEntity> spawned)
			    : m_name(std::move(name)), m_entities(std::move(spawned))
			{
			}

			void Undo() override
			{
				ClearSelectionIfIn(GuidsOf(m_entities));
				DestroySnapshot(m_entities);
			}

			void Redo() override
			{
				Entity root = RestoreEntities(m_entities);
				if (root && root.IsValid()) {
					GetUI().m_selectedEntity = root;
				}
			}

			const char* Name() const override { return m_name.c_str(); }

		  private:
			std::string                   m_name;
			std::vector<SerializedEntity> m_entities;
		};

		class DestroyEntitiesCommand final : public IUndoCommand {
		  public:
			DestroyEntitiesCommand(std::string name, std::vector<SerializedEntity> destroyed)
			    : m_name(std::move(name)), m_entities(std::move(destroyed))
			{
			}

			void Undo() override
			{
				Entity root = RestoreEntities(m_entities);
				if (root && root.IsValid()) {
					GetUI().m_selectedEntity = root;
				}
			}

			void Redo() override
			{
				ClearSelectionIfIn(GuidsOf(m_entities));
				DestroySnapshot(m_entities);
			}

			const char* Name() const override { return m_name.c_str(); }

		  private:
			std::string                   m_name;
			std::vector<SerializedEntity> m_entities;
		};

	} // namespace

	UndoHistory& GetUndo()
	{
		static UndoHistory history;
		return history;
	}

	bool CurrentWindowOwnsInteraction()
	{
		ImGuiContext& g   = *GImGui;
		ImGuiWindow*  cur = ImGui::GetCurrentWindow();
		if (!cur) {
			return false;
		}

		auto belongs = [&](ImGuiWindow* window) {
			for (; window; window = window->ParentWindow) {
				if (window == cur) {
					return true;
				}
			}
			return false;
		};

		if (g.ActiveId != 0 && belongs(g.ActiveIdWindow)) {
			return true;
		}

		// Asset pickers / add-component live in popups parented to this window.
		if (g.ActiveId != 0 && g.ActiveIdWindow && belongs(g.ActiveIdWindow->ParentWindow)) {
			return true;
		}

		if (ImGui::IsPopupOpen(static_cast<ImGuiID>(0), ImGuiPopupFlags_AnyPopupId) &&
		    ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
			return true;
		}
		return false;
	}

	SerializedEntity UndoHistory::CaptureEntity(Entity entity)
	{
		SerializedEntity se;
		if (!entity || !entity.IsValid() || !entity.HasComponent<Components::EntityMetadata>()) {
			return se;
		}

		if (entity.HasComponent<Components::LuaScript>()) {
			entity.GetComponent<Components::LuaScript>().SyncFromLua();
		}

		se.meta = entity.GetComponent<Components::EntityMetadata>();

#define X(type, name, fancy)                                                                                                                                                                                                                   \
		if (entity.HasComponent<type>()) {                                                                                                                                                                                                     \
			type snap{};                                                                                                                                                                                                                       \
			CopySerializedFields(entity.GetComponent<type>(), snap);                                                                                                                                                                           \
			se.name = std::move(snap);                                                                                                                                                                                                         \
		}
		COMPONENT_LIST
#undef X
		return se;
	}

	std::vector<SerializedEntity> UndoHistory::CaptureSubtree(Entity root)
	{
		std::vector<SerializedEntity>   out;
		std::vector<Entity>             entities;
		std::unordered_set<std::string> visited;
		CollectSubtree(root, entities, visited);
		out.reserve(entities.size());
		for (Entity& e : entities) {
			out.push_back(CaptureEntity(e));
		}
		return out;
	}

	void UndoHistory::DiscardRedo()
	{
		if (m_index + 1 < static_cast<int>(m_stack.size())) {
			m_stack.erase(m_stack.begin() + m_index + 1, m_stack.end());
			if (m_saveValid && m_savedIndex > m_index) {
				m_saveValid = false;
			}
		}
	}

	void UndoHistory::Push(std::unique_ptr<IUndoCommand> cmd)
	{
		if (!cmd || m_applying) {
			return;
		}
		DiscardRedo();
		m_stack.push_back(std::move(cmd));
		if (static_cast<int>(m_stack.size()) > kMaxCommands) {
			const int extra = static_cast<int>(m_stack.size()) - kMaxCommands;
			m_stack.erase(m_stack.begin(), m_stack.begin() + extra);
			if (m_saveValid && m_savedIndex >= 0) {
				m_savedIndex -= extra;
				if (m_savedIndex < 0) {
					m_saveValid = false;
				}
			}
		}
		m_index = static_cast<int>(m_stack.size()) - 1;
		RefreshDirtyFlag();
	}

	void UndoHistory::SyncTracker(const SerializedEntity& state)
	{
		m_lastClean      = state;
		m_lastCleanValid = true;
		m_trackGuid      = state.meta.guid;
		m_open           = false;
	}

	void UndoHistory::CommitOpenSession()
	{
		if (!m_open) {
			return;
		}
		if (!SameState(m_before, m_latest)) {
			std::string name = DescribeModify(m_before, m_latest);
			if (name == "Edit Entity" && !m_openName.empty() && m_openName != "Edit Entity") {
				name = m_openName;
			}
			Push(std::make_unique<ModifyEntityCommand>(std::move(name), m_before, m_latest));
		}
		m_lastClean      = m_latest;
		m_lastCleanValid = true;
		m_open           = false;
		RefreshDirtyFlag();
	}

	void UndoHistory::CancelOpenSession()
	{
		if (!m_open) {
			return;
		}
		m_applying = true;
		Scene* scene = GetCurrentScene();
		if (scene) {
			const EntityHandle handle(m_before.meta.guid);
			if (scene->m_entityMap.count(handle)) {
				Entity entity = scene->Get(handle);
				if (entity && entity.IsValid()) {
					ApplyEntityState(entity, m_before);
				}
			}
		}
		m_applying       = false;
		m_lastClean      = m_before;
		m_lastCleanValid = true;
		m_open           = false;
		RefreshDirtyFlag();
	}

	void UndoHistory::NotifyInteracting(bool interacting, const char* actionName)
	{
		if (interacting) {
			m_interacting = true;
			if (actionName && actionName[0] != '\0') {
				m_pendingName = actionName;
			}
		}
	}

	void UndoHistory::Flush(Entity selected)
	{
		if (m_applying || GetState() != EDITOR) {
			m_interacting = false;
			return;
		}

		if (!selected || !selected.IsValid() || !selected.HasComponent<Components::EntityMetadata>()) {
			CommitOpenSession();
			m_lastCleanValid = false;
			m_trackGuid.clear();
			m_interacting = false;
			return;
		}

		const std::string guid = selected.GetComponent<Components::EntityMetadata>().guid;
		if (guid != m_trackGuid) {
			CommitOpenSession();
			m_trackGuid      = guid;
			m_lastClean      = CaptureEntity(selected);
			m_lastCleanValid = true;
			m_open           = false;
			m_interacting    = false;
			return;
		}

		SerializedEntity now = CaptureEntity(selected);
		if (!m_lastCleanValid) {
			m_lastClean      = now;
			m_lastCleanValid = true;
		}

		if (!SameState(now, m_lastClean)) {
			if (!m_open) {
				m_open     = true;
				m_before   = m_lastClean;
				m_openName = !m_pendingName.empty() ? m_pendingName : "Edit Entity";
			}
			m_latest = now;
		}
		m_pendingName.clear();

		if (m_open && !m_interacting) {
			CommitOpenSession();
		}
		else if (!m_open) {
			m_lastClean = now;
		}

		m_interacting = false;
	}

	void UndoHistory::RecordModify(Entity entity, const SerializedEntity& before, const char* name)
	{
		if (m_applying || !entity || !entity.IsValid()) {
			return;
		}
		SerializedEntity after = CaptureEntity(entity);
		if (SameState(before, after)) {
			return;
		}
		m_open = false;
		std::string resolved = (name && name[0] != '\0' && std::strcmp(name, "Edit Entity") != 0)
		                           ? std::string(name)
		                           : DescribeModify(before, after);
		Push(std::make_unique<ModifyEntityCommand>(std::move(resolved), before, after));
		SyncTracker(after);
	}

	void UndoHistory::RecordSpawned(Entity root, const char* name)
	{
		if (m_applying || !root || !root.IsValid()) {
			return;
		}
		m_open = false;
		auto snap = CaptureSubtree(root);
		if (snap.empty()) {
			return;
		}
		Push(std::make_unique<SpawnEntitiesCommand>(name ? name : "Create Entity", std::move(snap)));
		if (root.IsValid()) {
			SyncTracker(CaptureEntity(root));
		}
	}

	void UndoHistory::DestroyAndRecord(Entity root, const char* name)
	{
		if (m_applying || !root || !root.IsValid()) {
			return;
		}
		m_open      = false;
		auto snap   = CaptureSubtree(root);
		if (snap.empty()) {
			return;
		}
		const std::unordered_set<std::string> guids = GuidsOf(snap);
		ClearSelectionIfIn(guids);
		DestroySnapshot(snap);
		Push(std::make_unique<DestroyEntitiesCommand>(name ? name : "Delete Entity", std::move(snap)));
		m_lastCleanValid = false;
		m_trackGuid.clear();
	}

	void UndoHistory::Undo()
	{
		if (GetState() != EDITOR) {
			return;
		}
		if (m_open) {
			CancelOpenSession();
			return;
		}
		if (!CanUndo()) {
			return;
		}
		m_applying = true;
		m_stack[static_cast<size_t>(m_index)]->Undo();
		--m_index;
		m_applying = false;
		InvalidateTracker();
		RefreshDirtyFlag();
	}

	void UndoHistory::Redo()
	{
		if (GetState() != EDITOR) {
			return;
		}
		CommitOpenSession();
		if (!CanRedo()) {
			return;
		}
		++m_index;
		m_applying = true;
		m_stack[static_cast<size_t>(m_index)]->Redo();
		m_applying = false;
		InvalidateTracker();
		RefreshDirtyFlag();
	}

	bool UndoHistory::CanUndo() const
	{
		if (GetState() != EDITOR) {
			return false;
		}
		return m_open || m_index >= 0;
	}

	bool UndoHistory::CanRedo() const
	{
		if (GetState() != EDITOR) {
			return false;
		}
		return !m_open && m_index + 1 < static_cast<int>(m_stack.size());
	}

	const char* UndoHistory::UndoLabel() const
	{
		if (m_open) {
			return m_openName.c_str();
		}
		if (m_index >= 0 && m_index < static_cast<int>(m_stack.size())) {
			return m_stack[static_cast<size_t>(m_index)]->Name();
		}
		return "Undo";
	}

	const char* UndoHistory::RedoLabel() const
	{
		const int next = m_index + 1;
		if (next >= 0 && next < static_cast<int>(m_stack.size())) {
			return m_stack[static_cast<size_t>(next)]->Name();
		}
		return "Redo";
	}

	void UndoHistory::Clear()
	{
		m_stack.clear();
		m_index          = -1;
		m_savedIndex     = -1;
		m_saveValid      = true;
		m_open           = false;
		m_interacting    = false;
		m_pendingName.clear();
		InvalidateTracker();
	}

	void UndoHistory::InvalidateTracker()
	{
		m_lastCleanValid = false;
		m_trackGuid.clear();
	}

	void UndoHistory::RefreshDirtyFlag()
	{
		if (m_saveValid && m_index == m_savedIndex && !m_open) {
			GetEditor().ClearDirty();
		}
		else {
			GetEditor().MarkDirty();
		}
	}

	void UndoHistory::PrepareForPlay()
	{
		CommitOpenSession();
		InvalidateTracker();
		m_interacting = false;
	}

	void UndoHistory::RestoreAfterStop()
	{
		InvalidateTracker();
		m_interacting = false;
		m_open        = false;
	}

	void UndoHistory::MarkSaved()
	{
		m_savedIndex = m_index;
		m_saveValid  = true;
		GetEditor().ClearDirty();
	}

	void UndoHistory::InvalidateSavePoint()
	{
		m_saveValid = false;
	}

} // namespace Engine::UI

#else // GAME_BUILD

namespace Engine::UI {

	UndoHistory& GetUndo()
	{
		static UndoHistory history;
		return history;
	}

	bool CurrentWindowOwnsInteraction() { return false; }

	SerializedEntity UndoHistory::CaptureEntity(Entity) { return {}; }
	std::vector<SerializedEntity> UndoHistory::CaptureSubtree(Entity) { return {}; }

	void UndoHistory::Undo() {}
	void UndoHistory::Redo() {}
	bool UndoHistory::CanUndo() const { return false; }
	bool UndoHistory::CanRedo() const { return false; }
	const char* UndoHistory::UndoLabel() const { return "Undo"; }
	const char* UndoHistory::RedoLabel() const { return "Redo"; }
	void UndoHistory::Clear() {}
	void UndoHistory::PrepareForPlay() {}
	void UndoHistory::RestoreAfterStop() {}
	void UndoHistory::MarkSaved() {}
	void UndoHistory::InvalidateSavePoint() {}
	void UndoHistory::NotifyInteracting(bool, const char*) {}
	void UndoHistory::Flush(Entity) {}
	void UndoHistory::RecordModify(Entity, const SerializedEntity&, const char*) {}
	void UndoHistory::RecordSpawned(Entity, const char*) {}
	void UndoHistory::DestroyAndRecord(Entity, const char*) {}
	void UndoHistory::Push(std::unique_ptr<IUndoCommand>) {}
	void UndoHistory::DiscardRedo() {}
	void UndoHistory::CommitOpenSession() {}
	void UndoHistory::CancelOpenSession() {}
	void UndoHistory::SyncTracker(const SerializedEntity&) {}
	void UndoHistory::InvalidateTracker() {}
	void UndoHistory::RefreshDirtyFlag() {}

} // namespace Engine::UI

#endif
