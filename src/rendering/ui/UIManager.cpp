#include "UIManager.h"
#include "EditorSession.h"

#include "components/Components.h"
#include "components/AllComponents.h"
#include "core/EngineData.h"

#include "rendering/Renderer.h"
#include "rendering/ui/GameUIManager.h"

#include "rendering/ui/IconsFontAwesome6.h"
#include "rendering/ui/Themes.h"

#include "rendering/ui/windows/SceneViewWindow.h"
#include "rendering/particles/ParticleManager.h"
#include "components/impl/EntityMetadataComponent.h"
#include "components/impl/TerrainRendererComponent.h"


#include "assets/impl/JSONSceneLoader.h"

#include "physics/PhysicsManager.h"

#include "core/Input.h"
#include "core/SceneManager.h"
#include "core/module/ModuleManager.h"
#include "scripting/ScriptManager.h"

#include "windows/ConsoleWindow.h"
#include "windows/AudioDebugWindow.h"
#include "windows/SceneViewWindow.h"
#include "windows/AnimationWindow.h"

#include "utils/Builder.h"
#include "components/impl/ModelRendererComponent.h"
#include "rendering/particles/Particle.h"
#include "components/impl/ParticleSystemComponent.h"
#include "assets/impl/MaterialLoader.h"
#include "assets/Prefab.h"


#include <nfd.h>
#include <cstdio>

#include <RmlUi/Core.h>

std::string SelectFolder()
{
	nfdchar_t*  folder = nullptr;
	nfdresult_t result = NFD_PickFolder(nullptr, &folder);

	if (result == NFD_OKAY) {
		std::string path(folder);
		free(folder);
		return path;
	}
	else if (result == NFD_CANCEL) {
		return ""; // user cancelled
	}
	else {
		std::cerr << "NFD Error: " << NFD_GetError() << std::endl;
		return "";
	}
}


namespace Engine::UI {


	void UIManager::onShutdown()
	{
	}


	void UIManager::onInit()
	{
        ZoneScopedN("Initialize Editor UI");
		SetThemeColors(0);

		m_uiAssetRenderer   = std::make_unique<AssetUIRenderer>();
		m_materialEditor    = std::make_unique<MaterialEditor>();
		m_inspectorRenderer = std::make_unique<InspectorRenderer>();

		m_audioIconTexture     = std::make_shared<Texture>();
		m_terrainIconTexture   = std::make_shared<Texture>();
		m_animationIconTexture = std::make_shared<Texture>();
		m_skeletonIconTexture  = std::make_shared<Texture>();
		m_folderIconTexture    = std::make_shared<Texture>();
		m_fileIconTexture      = std::make_shared<Texture>();
		m_modelIconTexture     = std::make_shared<Texture>();
		m_shaderIconTexture    = std::make_shared<Texture>();
		m_particleIconTexture  = std::make_shared<Texture>();
		m_materialIconTexture  = std::make_shared<Texture>();
#ifndef GAME_BUILD
		m_audioIconTexture->LoadFromFile("resources/engine/speaker.png");
		m_terrainIconTexture->LoadFromFile("resources/engine/mountain.png");
		m_animationIconTexture->LoadFromFile("resources/engine/animation.png");
		m_skeletonIconTexture->LoadFromFile("resources/engine/skeleton.png");
		m_folderIconTexture->LoadFromFile("resources/engine/folder.png");
		m_fileIconTexture->LoadFromFile("resources/engine/file.png");
		m_modelIconTexture->LoadFromFile("resources/engine/model.png");
		m_shaderIconTexture->LoadFromFile("resources/engine/shader.png");
		m_particleIconTexture->LoadFromFile("resources/engine/particle.png");
		m_materialIconTexture->LoadFromFile("resources/engine/material_icon.png");

		efsw::WatchID id = m_uiAssetRenderer->fw.addWatch("resources", &m_uiAssetRenderer->listener, true);
		m_uiAssetRenderer->fw.watch();
#endif

	// Note: RmlUi demo document will be loaded in Window after Lua plugin is initialized
	}


	void UIManager::BeginDockspace(float height)
	{
		SetThemeColors(m_selectedTheme);

		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking;

		const ImGuiViewport* viewport = ImGui::GetMainViewport();

		ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + height));
		ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, viewport->Size.y - height));
		ImGui::SetNextWindowViewport(viewport->ID);


		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

		windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

		bool dockspaceOpen = true;
		ImGui::Begin("Dockspace", &dockspaceOpen, windowFlags);
		ImGui::PopStyleVar(3);

		ImGuiID dockspaceID = ImGui::GetID("Dockspace");

		ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
	}


	void UIManager::EndDockspace()
	{
		ImGui::End();
	}


	void Play()
	{
#ifndef GAME_BUILD
		GetEditor().Play();
#endif
	}

	void Pause()
	{
#ifndef GAME_BUILD
		GetEditor().Pause();
#endif
	}

	void Stop()
	{
#ifndef GAME_BUILD
		GetEditor().Stop();
#endif
	}


	float UIManager::RenderMainMenuBar()
	{
		float height = 0.0f;
		ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0, 0, 0, 0));
		if (ImGui::BeginMainMenuBar()) {
			auto& editor = GetEditor();
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("New Scene", "Ctrl+N")) editor.NewScene();
				if (ImGui::MenuItem("Open Scene...", "Ctrl+O")) editor.OpenSceneDialog();
				if (ImGui::MenuItem("Save Scene", "Ctrl+S")) editor.SaveScene();
				if (ImGui::MenuItem("Save Scene As...")) editor.SaveSceneAs();
				if (ImGui::MenuItem("Save Selection as Prefab...", nullptr, false, m_selectedEntity && m_selectedEntity.IsValid())) {
					editor.SaveEntityAsPrefab(m_selectedEntity);
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Build Game")) {
					std::string path = SelectFolder();
					BuildGame(path);
				}

				ImGui::Separator();
				if (ImGui::MenuItem("Exit")) {
					glfwSetWindowShouldClose(GetWindow().GetNativeWindow(), GLFW_TRUE);
				}
				ImGui::EndMenu();
			}


			if (ImGui::BeginMenu("View")) {
				ImGui::MenuItem("Hierarchy", nullptr, &editor.showHierarchy);
				ImGui::MenuItem("Inspector", nullptr, &editor.showInspector);
				ImGui::MenuItem("Assets", nullptr, &editor.showAssets);
				ImGui::MenuItem("Console", nullptr, &editor.showConsole);
				ImGui::MenuItem("Material Editor", nullptr, &editor.showMaterialEditor);
				ImGui::Separator();
				ImGui::MenuItem("Animation", nullptr, &editor.showAnimation);
				ImGui::MenuItem("Audio Debug", nullptr, &editor.showAudioDebug);
				ImGui::MenuItem("GBuffer Debug", nullptr, &editor.showGBufferDebug);
				ImGui::MenuItem("Model Debug", nullptr, &editor.showModelDebug);
				ImGui::Separator();
				ImGui::MenuItem("Editor Settings", nullptr, &editor.showSettings);
				static bool showDemo = false;
				if (ImGui::MenuItem("ImGui Demo", nullptr, showDemo)) showDemo = !showDemo;
				if (showDemo) ImGui::ShowDemoWindow(&showDemo);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Help")) {
				ImGui::MenuItem("Keyboard Shortcuts", "F1", &editor.showShortcuts);
				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();

			height = ImGui::GetFrameHeight(); // main menu bar height
		}
		ImGui::PopStyleColor();


		return height;
	}


	float UIManager::RenderTopBar(float top)
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();

		ImVec2 np = ImVec2(viewport->Pos.x, viewport->Pos.y + top);
		ImGui::SetNextWindowPos(np);
		ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 32.0f));
		ImGui::SetNextWindowViewport(viewport->ID);


		// Style: no title, no scroll, no resize, no move, no collapse, no docking
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
		                                ImGuiWindowFlags_NoDocking;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 4));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 0));

		if (ImGui::Begin("##TopBar", nullptr, window_flags)) {
#define TOOLBUTTON(name, type)                                                                                                                                                                                                                 \
	if (SceneViewWindow::mCurrentGizmoOperation == ImGuizmo::type) {                                                                                                                                                                           \
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25, 0.25, 0.75, 1.0));                                                                                                                                                                 \
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35, 0.35, 0.85, 1.0));                                                                                                                                                          \
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.45, 0.45, 0.95, 1.0));                                                                                                                                                           \
	}                                                                                                                                                                                                                                          \
	bool Tool##type = ImGui::Button(name);                                                                                                                                                                                                     \
	if (SceneViewWindow::mCurrentGizmoOperation == ImGuizmo::type) {                                                                                                                                                                           \
		ImGui::PopStyleColor(3);                                                                                                                                                                                                               \
	}                                                                                                                                                                                                                                          \
	if (Tool##type) {                                                                                                                                                                                                                          \
		SceneViewWindow::mCurrentGizmoOperation = ImGuizmo::type;                                                                                                                                                                              \
	}


			TOOLBUTTON(ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT "##tooltranslate", TRANSLATE)
			ImGui::SameLine();
			TOOLBUTTON(ICON_FA_ROTATE "##toolrotate", ROTATE)
			ImGui::SameLine();
			TOOLBUTTON(ICON_FA_UP_RIGHT_AND_DOWN_LEFT_FROM_CENTER "##toolscale", SCALE)
			ImGui::SameLine();
			TOOLBUTTON(ICON_FA_BORDER_TOP_LEFT "##toolscalebounds", BOUNDS)


#undef TOOLBUTTON

			ImGui::SameLine(185);


			const char* modeNames[] = {"Local", "World"};
			int         current     = (SceneViewWindow::mCurrentGizmoMode == ImGuizmo::MODE::LOCAL ? 0 : 1);
			ImGui::SetNextItemWidth(80);
			if (ImGui::BeginCombo("##WorldMode", modeNames[current])) {
				for (int i = 0; i < 2; i++) {
					bool isSelected = (current == i);
					if (ImGui::Selectable(modeNames[i], isSelected)) current = i;

					if (isSelected) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			SceneViewWindow::mCurrentGizmoMode = (current == 0 ? ImGuizmo::MODE::LOCAL : ImGuizmo::MODE::WORLD);

			ImGui::SameLine();
			auto& editor = GetEditor();
			ImGui::Checkbox("Snap", &editor.snapEnabled);
			if (editor.snapEnabled) {
				ImGui::SameLine();
				ImGui::SetNextItemWidth(50);
				ImGui::DragFloat("##snapT", &editor.snapTranslate, 0.05f, 0.01f, 10.0f, "%.2f");
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Translate snap");
			}


			// --- Center Play Controls ---
			float windowWidth        = ImGui::GetWindowSize().x;
			float playBtnWidth       = ImGui::CalcTextSize(ICON_FA_PLAY).x + ImGui::GetStyle().FramePadding.x * 2;
			float pauseBtnWidth      = ImGui::CalcTextSize(ICON_FA_PAUSE).x + ImGui::GetStyle().FramePadding.x * 2;
			float stopBtnWidth       = ImGui::CalcTextSize(ICON_FA_STOP).x + ImGui::GetStyle().FramePadding.x * 2;
			float totalControlsWidth = playBtnWidth + pauseBtnWidth + stopBtnWidth + ImGui::GetStyle().ItemSpacing.x * 2;

			float cursorX = (windowWidth - totalControlsWidth) * 0.5f;
			ImGui::SameLine();
			ImGui::SetCursorPosX(cursorX);


			if (GetState() == PLAYING) {
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);                        // Disable input
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.25f); // Dim look
			}
			bool startPlay = ImGui::Button(ICON_FA_PLAY "##play");
			if (GetState() == PLAYING) {
				ImGui::PopStyleVar();
				ImGui::PopItemFlag();
			}
			if (startPlay) {
				Play();
			}
			ImGui::SameLine();

			if (GetState() != PLAYING) {
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);                        // Disable input
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.25f); // Dim look
			}
			bool startPause = ImGui::Button(ICON_FA_PAUSE "##pause");
			if (GetState() != PLAYING) {
				ImGui::PopStyleVar();
				ImGui::PopItemFlag();
			}
			if (startPause) {
				Pause();
			}


			ImGui::SameLine();


			if (GetState() == EDITOR) {
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);                        // Disable input
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.25f); // Dim look
			}
			bool startStop = ImGui::Button(ICON_FA_STOP "##stop");
			if (GetState() == EDITOR) {
				ImGui::PopStyleVar();
				ImGui::PopItemFlag();
			}
			if (startStop) {
				Stop();
			}

			ImGui::SameLine();
			const bool canStep = GetState() == PAUSED || GetState() == PLAYING;
			if (!canStep) {
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.25f);
			}
			if (ImGui::Button(ICON_FA_FORWARD_STEP "##step")) {
				GetEditor().Step();
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Step one frame (F10 while paused)");
			if (!canStep) {
				ImGui::PopStyleVar();
				ImGui::PopItemFlag();
			}
		}

		float barHeight = ImGui::GetWindowHeight();
		ImGui::End();
		ImGui::PopStyleVar(3);

		return barHeight;
	}

	bool consoleOpen = true;

	Entity UIManager::DuplicateEntity(Entity source)
	{
		if (!source || !source.IsValid()) {
			return Entity();
		}

		std::string newName = source.GetName();
		if (newName.rfind("Copy of ", 0) != 0) {
			newName = "Copy of " + newName;
		}

		Entity copy = Entity::Create(newName, source.m_scene);

#define X(type, name, fancy)                                                                                                                                                                                                                   \
		if (source.HasComponent<type>()) {                                                                                                                                                                                                     \
			copy.AddComponent<type>(source.GetComponent<type>());                                                                                                                                                                              \
		}
		COMPONENT_LIST
#undef X

		auto& srcMeta = source.GetComponent<Components::EntityMetadata>();
		if (srcMeta.parentEntity.IsValid()) {
			copy.SetParent(srcMeta.parentEntity);
		}

		if (copy.HasComponent<Components::Transform>()) {
			auto& tr = copy.GetComponent<Components::Transform>();
			tr.SetLocalPosition(tr.GetLocalPosition() + glm::vec3(1.0f, 0.0f, 0.0f));
		}

		GetEditor().MarkDirty();
		return copy;
	}

	void UIManager::FlushHierarchyCommands()
	{
		if (m_hierarchyCommand == HierarchyCommand::None || !m_hierarchyCommandEntity.IsValid()) {
			m_hierarchyCommand       = HierarchyCommand::None;
			m_hierarchyCommandEntity = Entity();
			return;
		}

		Entity target = m_hierarchyCommandEntity;
		const HierarchyCommand cmd = m_hierarchyCommand;
		m_hierarchyCommand       = HierarchyCommand::None;
		m_hierarchyCommandEntity = Entity();

		switch (cmd) {
			case HierarchyCommand::Delete:
				if (m_selectedEntity == target) {
					m_selectedEntity = Entity();
				}
				if (target.HasComponent<Components::EntityMetadata>() &&
				    m_renamingGuid == target.GetComponent<Components::EntityMetadata>().guid) {
					m_renamingGuid.clear();
				}
				target.Destroy();
				break;
			case HierarchyCommand::Duplicate:
				m_selectedEntity = DuplicateEntity(target);
				break;
			case HierarchyCommand::CreateChild: {
				Entity child = Entity::Create("New Entity", target.m_scene);
				child.SetParent(target.GetEntityHandle());
				m_selectedEntity = child;
				GetEditor().MarkDirty();
				break;
			}
			case HierarchyCommand::SavePrefab:
				GetEditor().SaveEntityAsPrefab(target);
				break;
			case HierarchyCommand::InstantiatePrefab: {
				Entity spawned = GetEditor().InstantiatePrefabDialog(target.GetEntityHandle());
				if (spawned && spawned.IsValid()) {
					m_selectedEntity = spawned;
				}
				break;
			}
			case HierarchyCommand::None:
				break;
		}
	}

	void UIManager::onUpdate(float dt)
	{
        ZoneScopedN("OnUpdate UI manager");
		auto& editor = GetEditor();
		m_selectedTheme = editor.theme;
		editor.HandleShortcuts();

		float h      = RenderMainMenuBar();
		float height = RenderTopBar(h) + h;
		BeginDockspace(height);

		m_overSceneView = SceneViewWindow::DrawSceneViewWindow();
		if (editor.showHierarchy) RenderHierarchyWindow();
		if (editor.showInspector) m_inspectorRenderer->RenderInspectorWindow(&m_selectedEntity);
		if (editor.showMaterialEditor) m_materialEditor->RenderMaterialEditor(m_selectedMaterial);

		if (editor.showAnimation) DrawAnimationWindow();
		if (editor.showAudioDebug) DrawAudioDebugWindow();
        if (editor.showModelDebug) RenderModelDebug(m_selectedModel);
        if (editor.showGBufferDebug) RenderGBufferDebug(GetWindow().GetGBuffer());
		if (editor.showConsole) DrawConsoleWindow(Logger::getImGuiSink(), &editor.showConsole);

		if (editor.showAssets) m_uiAssetRenderer->RenderAssetWindow();

		editor.DrawSettingsWindow();
		editor.DrawShortcutsOverlay();
		editor.DrawConfirmModals();

		if (GetState() == PLAYING) {
			ImGui::SetNextWindowPos(ImVec2(GetWindow().targetX + 10, GetWindow().targetY + 10), ImGuiCond_Always);
			ImGui::SetNextWindowBgAlpha(0.45f);
			ImGui::Begin("PlayBanner", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove);
			ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.5f, 1.0f), "PLAYING");
			ImGui::End();
		}
		if (GetState() == PAUSED) {
			RenderPauseOverlay();
		}

		EndDockspace();
	}

	static const char* HierarchyTypeIcon(Entity entity)
	{
		if (entity.HasComponent<Components::ModelRenderer>()) return ICON_FA_CUBE;
		if (entity.HasComponent<Components::SkinnedMeshComponent>()) return ICON_FA_PERSON;
		if (entity.HasComponent<Components::AudioSource>()) return ICON_FA_VOLUME_HIGH;
		if (entity.HasComponent<Components::LuaScript>()) return ICON_FA_SCROLL;
		if (entity.HasComponent<Components::ParticleSystem>()) return ICON_FA_STAR;
		if (entity.HasComponent<Components::TerrainRenderer>()) return ICON_FA_MOUNTAIN;
		if (entity.HasComponent<Components::ShadowCaster>()) return ICON_FA_MOON;
		if (entity.HasComponent<Components::PlayerControllerComponent>()) return ICON_FA_GAMEPAD;
		if (entity.HasComponent<Components::Text3DComponent>()) return ICON_FA_FONT;
		if (entity.HasComponent<Components::RmlUIComponent>()) return ICON_FA_WINDOW_MAXIMIZE;
		if (entity.HasComponent<Components::AnimationComponent>()) return ICON_FA_FILM;
		if (entity.HasComponent<Components::PrefabInstance>()) return ICON_FA_BOX_OPEN;
		return ICON_FA_CUBE;
	}

	static bool HierarchyMatchesOrHasMatch(Entity entity, const char* filter)
	{
		if (!filter || filter[0] == '\0') return true;
		if (!entity.HasComponent<Components::EntityMetadata>()) return false;
		auto& meta = entity.GetComponent<Components::EntityMetadata>();
		if (EditorSession::MatchesFilter(meta.name, filter) || EditorSession::MatchesFilter(meta.tag, filter)) {
			return true;
		}
		for (auto& childHandle : meta.children) {
			Entity child = GetCurrentScene()->Get(childHandle);
			if (child && HierarchyMatchesOrHasMatch(child, filter)) return true;
		}
		return false;
	}

	void UIManager::DrawAddEntityMenu()
	{
		ImGui::Text("Add Entity");
		ImGui::Separator();
		if (ImGui::MenuItem("Empty Entity")) {
			Entity entity    = Entity::Create("New Entity", GetCurrentScene());
			m_selectedEntity = entity;
			GetEditor().MarkDirty();
		}

		glm::vec3 position = GetCamera().GetPosition();
		glm::vec3 forward  = GetCamera().GetFront();
		glm::vec3 spawnPos = position + glm::vec3(forward.x * 3, forward.y * 3, forward.z * 3);

		if (ImGui::MenuItem("Add Model")) {
			Entity entity = Entity::Create("New Model", GetCurrentScene());
			entity.AddComponent<Components::Transform>(spawnPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
			entity.AddComponent<Components::ModelRenderer>();
			m_selectedEntity = entity;
			GetEditor().MarkDirty();
		}
		if (ImGui::MenuItem("Add Particle System")) {
			Entity entity = Entity::Create("New Particle System", GetCurrentScene());
			entity.AddComponent<Components::Transform>(spawnPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
			entity.AddComponent<Components::ParticleSystem>();
			m_selectedEntity = entity;
			GetEditor().MarkDirty();
		}
		if (ImGui::MenuItem("Instantiate Prefab...")) {
			Entity spawned = GetEditor().InstantiatePrefabDialog();
			if (spawned && spawned.IsValid()) {
				m_selectedEntity = spawned;
			}
		}
	}

	void UIManager::RenderHierarchyWindow()
	{
		ImGui::Begin("Hierarchy");

		auto& editor = GetEditor();
		ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::InputTextWithHint("##hier_search", ICON_FA_MAGNIFYING_GLASS " Search...", editor.hierarchyFilter, IM_ARRAYSIZE(editor.hierarchyFilter));

		// Get all entities with EntityMetadata component
		auto view = GetCurrentSceneRegistry().view<Components::EntityMetadata>();

		bool         changeParent = false;
		Entity       _newChild;
		EntityHandle _newParent;


		// Render only root entities (those without parents)
		for (auto entity : view) {
			Entity e(entity, GetCurrentScene());
			auto&  metadata = e.GetComponent<Components::EntityMetadata>();

			// Only render root entities here
			if (!metadata.parentEntity.IsValid()) {
				if (HierarchyMatchesOrHasMatch(e, editor.hierarchyFilter)) {
					RenderEntityTreeNode(e);
				}
			}
		}

		// Fill leftover space so right-click / drop work all the way to the bottom.
		// ContextWindow + NoOpenOverItems cannot open on this button, so the menu
		// is attached to the drop zone itself.
		ImVec2 contentRegionAvail = ImGui::GetContentRegionAvail();
		if (contentRegionAvail.y < 8.0f) {
			contentRegionAvail.y = 8.0f;
		}
		ImGui::InvisibleButton("##HierarchyDropZone", contentRegionAvail);

		if (ImGui::BeginDragDropTarget()) {
			struct PayloadData {
				const char* type;
				char        id[64];
			};
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_HANDLE")) {
				if (payload->DataSize == sizeof(PayloadData)) {
					const auto* data = static_cast<const PayloadData*>(payload->Data);
					if (std::strcmp(data->type, "EntityHandle") == 0) {
						std::string draggedGuid = data->id;
						log->info("SETTING PARENT {}", draggedGuid);

						EntityHandle parentHandle = EntityHandle();
						EntityHandle childHandle  = EntityHandle(draggedGuid);
						Entity       child        = GetCurrentScene()->Get(childHandle);

						_newChild    = child;
						_newParent   = parentHandle;
						changeParent = true;
					}
				}
			}
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PREFAB")) {
				if (payload->DataSize == sizeof(PayloadData)) {
					const auto* data = static_cast<const PayloadData*>(payload->Data);
					Entity spawned = InstantiatePrefab(PrefabHandle(data->id));
					if (spawned && spawned.IsValid()) {
						m_selectedEntity = spawned;
						GetEditor().MarkDirty();
					}
				}
			}
			ImGui::EndDragDropTarget();
		}

		if (ImGui::BeginPopupContextItem("HierarchyEmptyContext")) {
			DrawAddEntityMenu();
			ImGui::EndPopup();
		}

		if (ImGui::BeginPopupContextWindow("HierarchyContext", ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight)) {
			DrawAddEntityMenu();
			ImGui::EndPopup();
		}

		ImGui::End();
		if (changeParent) {
			_newChild.SetParent(_newParent);
		}

		FlushHierarchyCommands();
	}

	void UIManager::RenderEntityTreeNode(Entity entity)
	{
		bool         changeParent = false;
		Entity       _newChild;
		EntityHandle _newParent;

		auto& metadata = entity.GetComponent<Components::EntityMetadata>();

		// Use GUID for stable ID, display name separately
		std::string guid = metadata.guid;

		// Check if this entity is selected
		bool isSelected = (m_selectedEntity == entity);

		// Set up tree node flags
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
		if (isSelected) {
			flags |= ImGuiTreeNodeFlags_Selected;
		}

		// If entity has no children, make it a leaf node
		if (metadata.children.empty()) {
			flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
		}

		// Push ID using GUID to keep tree state stable when name changes
		ImGui::PushID(guid.c_str());

		const float btnW     = ImGui::GetFrameHeight();
		const float btnsWide = btnW * 2.0f + 4.0f;

		char label[256];
		std::snprintf(label, sizeof(label), "%s %s", HierarchyTypeIcon(entity), metadata.name.c_str());
		// Later eye/lock buttons overlap this row — allow them to take the click.
		ImGui::SetNextItemAllowOverlap();
		bool nodeOpen = ImGui::TreeNodeEx(label, flags);

		const ImVec2 nodeMin   = ImGui::GetItemRectMin();
		const ImVec2 nodeSize  = ImGui::GetItemRectSize();
		const ImVec2 afterNode = ImGui::GetCursorScreenPos();
		const bool   overBtns  = ImGui::GetMousePos().x >= (nodeMin.x + nodeSize.x - btnsWide);

		// Capture click against the tree node before drag-drop runs. Select on press
		// (not release) so dropping onto a parent does not steal selection, and skip
		// the expand arrow so OpenOnArrow still works.
		const bool clickedArrow = ImGui::IsItemToggledOpen();
		const bool leftClicked  = ImGui::IsItemClicked(ImGuiMouseButton_Left) && !clickedArrow && !overBtns;
		const bool rightClicked = ImGui::IsItemClicked(ImGuiMouseButton_Right) && !overBtns;

		// Handle drag source
		bool isDragging = false;
		if (ImGui::BeginDragDropSource()) {
			isDragging = true;
			struct PayloadData {
				const char* type;
				char        id[64];
			};
			PayloadData payload;
			payload.type = "EntityHandle";
			strncpy(payload.id, guid.c_str(), sizeof(payload.id));
			payload.id[sizeof(payload.id) - 1] = '\0';

			ImGui::SetDragDropPayload("ENTITY_HANDLE", &payload, sizeof(payload));
			ImGui::Text("%s", metadata.name.c_str());

			ImGui::EndDragDropSource();
		}


		if (ImGui::BeginDragDropTarget()) {
			struct PayloadData {
				const char* type;
				char        id[64];
			};
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_HANDLE")) {
				if (payload->DataSize == sizeof(PayloadData)) {
					const auto* data = static_cast<const PayloadData*>(payload->Data);
					if (std::strcmp(data->type, "EntityHandle") == 0) {
						std::string draggedGuid = data->id;
						log->info("SETTING PARENT {}", draggedGuid);

						EntityHandle parentHandle = EntityHandle(guid);
						EntityHandle childHandle  = EntityHandle(draggedGuid);

						Entity child = GetCurrentScene()->Get(childHandle);
						// child.SetParent(parentHandle);
						_newChild    = child;
						_newParent   = parentHandle;
						changeParent = true;
					}
				}
			}
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PREFAB")) {
				if (payload->DataSize == sizeof(PayloadData)) {
					const auto* data = static_cast<const PayloadData*>(payload->Data);
					Entity spawned = InstantiatePrefab(PrefabHandle(data->id), EntityHandle(guid));
					if (spawned && spawned.IsValid()) {
						m_selectedEntity = spawned;
						GetEditor().MarkDirty();
					}
				}
			}
			ImGui::EndDragDropTarget();
		}

		// Select on press. While a drag is active, BeginDragDropSource is true so we
		// skip — that keeps reparent drops from changing selection to the target.
		if (!isDragging && (leftClicked || rightClicked)) {
			m_selectedEntity = entity;
		}
		if (!isDragging && leftClicked && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
			m_renamingGuid = guid;
			strncpy(m_renameBuffer, metadata.name.c_str(), sizeof(m_renameBuffer) - 1);
			m_renameBuffer[sizeof(m_renameBuffer) - 1] = '\0';
			m_renameFocusRequested                     = true;
		}

		if (!isDragging) {
			if (!overBtns) {
				ImGui::OpenPopupOnItemClick("EntityContext", ImGuiPopupFlags_MouseButtonRight);
			}
			if (ImGui::BeginPopup("EntityContext")) {
				ImGui::TextUnformatted(metadata.name.c_str());
				ImGui::Separator();
				if (ImGui::MenuItem("Rename")) {
					m_renamingGuid = guid;
					strncpy(m_renameBuffer, metadata.name.c_str(), sizeof(m_renameBuffer) - 1);
					m_renameBuffer[sizeof(m_renameBuffer) - 1] = '\0';
					m_renameFocusRequested                     = true;
				}
				if (ImGui::MenuItem("Duplicate")) {
					m_hierarchyCommand       = HierarchyCommand::Duplicate;
					m_hierarchyCommandEntity = entity;
				}
				if (ImGui::MenuItem("Create Child")) {
					m_hierarchyCommand       = HierarchyCommand::CreateChild;
					m_hierarchyCommandEntity = entity;
				}
				if (ImGui::MenuItem("Instantiate Prefab as Child...")) {
					m_hierarchyCommand       = HierarchyCommand::InstantiatePrefab;
					m_hierarchyCommandEntity = entity;
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Save as Prefab...")) {
					m_hierarchyCommand       = HierarchyCommand::SavePrefab;
					m_hierarchyCommandEntity = entity;
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Delete")) {
					GetEditor().pendingDeleteEntity  = entity;
					GetEditor().pendingConfirmDelete = true;
				}
				ImGui::EndPopup();
			}
		}

		if (m_renamingGuid == guid) {
			const float labelPad = ImGui::GetTreeNodeToLabelSpacing();
			const float width    = nodeSize.x - labelPad;
			ImGui::SetCursorScreenPos(ImVec2(nodeMin.x + labelPad, nodeMin.y));
			ImGui::PushItemWidth(width > 40.0f ? width : 40.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 0.0f));
			if (m_renameFocusRequested) {
				ImGui::SetKeyboardFocusHere();
				m_renameFocusRequested = false;
			}
			const bool committed = ImGui::InputText("##hier_rename", m_renameBuffer, sizeof(m_renameBuffer),
			                                        ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
			if (committed) {
				entity.SetName(m_renameBuffer);
				m_renamingGuid.clear();
			}
			else if (ImGui::IsItemDeactivated()) {
				if (!ImGui::IsKeyDown(ImGuiKey_Escape)) {
					entity.SetName(m_renameBuffer);
				}
				m_renamingGuid.clear();
			}
			ImGui::PopStyleVar();
			ImGui::PopItemWidth();
			// Overlay must not shift child rows
			ImGui::SetCursorScreenPos(afterNode);
		}

		{
			ImGui::SetCursorScreenPos(ImVec2(nodeMin.x + nodeSize.x - btnsWide, nodeMin.y));
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.12f));
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
			if (ImGui::Button(metadata.active ? ICON_FA_EYE "##vis" : ICON_FA_EYE_SLASH "##vis", ImVec2(btnW, nodeSize.y))) {
				metadata.active = !metadata.active;
				GetEditor().MarkDirty();
			}
			ImGui::SameLine(0.0f, 4.0f);
			const bool locked = GetEditor().IsEntityLocked(entity);
			if (ImGui::Button(locked ? ICON_FA_LOCK "##lock" : ICON_FA_LOCK_OPEN "##lock", ImVec2(btnW, nodeSize.y))) {
				GetEditor().ToggleEntityLocked(entity);
			}
			ImGui::PopStyleVar();
			ImGui::PopStyleColor(2);
			ImGui::SetCursorScreenPos(afterNode);
		}

		// Recursively render children
		if (nodeOpen && !metadata.children.empty()) {
			const char* filter = GetEditor().hierarchyFilter;
			for (auto& childHandle : metadata.children) {
				auto childEntity = GetCurrentScene()->Get(childHandle);
				if (childEntity && HierarchyMatchesOrHasMatch(childEntity, filter)) {
					RenderEntityTreeNode(childEntity);
				}
			}
			ImGui::TreePop();
		}

		// Pop the ID
		ImGui::PopID();

		if (changeParent) {
			_newChild.SetParent(_newParent);
			GetEditor().MarkDirty();
		}
	}


	void UIManager::RenderPauseOverlay()
	{
		ImGui::SetNextWindowPos(ImVec2(GetWindow().targetX + GetWindow().targetWidth - 10, GetWindow().targetY + 10), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
		ImGui::SetNextWindowBgAlpha(0.35f);
		ImGui::Begin("PauseOverlay", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove);
		ImGui::Text("GAME PAUSED");
		ImGui::Text("Play to resume  |  Step for one frame");
		ImGui::End();
	}

    void UIManager::RenderModelDebug(AssetHandle<Engine::Rendering::Model> handle) {
        ImGui::Begin("Model Debug");


        if (!handle.IsValid()) {
            ImGui::Text("Invalid model selected.");
        }
        else {
            Rendering::Model* model = GetAssetManager().Get(handle);

            std::string name = model->m_name;
            ImGui::Text("Model: %s", name.c_str());
            ImGui::Text("Meshes: %d", model->GetMeshes().size());

            if(ImGui::TreeNode("Meshes")) {

                for (int i = 0; i < model->GetMeshes().size(); i++) {
                    char buff[256];
                    sprintf(buff, "Mesh %d", i);

                    if (ImGui::TreeNode(buff)) {
                        auto m = model->GetMeshes()[i];
                        ImGui::Text("VAO: %i", m->GetVAO());
                        ImGui::Text("VBO: %i", m->GetVBO());
                        ImGui::Text("EBO: %i", m->GetEBO());

                        ImGui::TreePop();
                    }

                }
                ImGui::TreePop();
            }


            ImGui::Separator();

            if(ImGui::Button("Extract Materials")) {
                auto exportPath = SelectFolder();

                if(!exportPath.empty()) {
                    log->info("Materials being extracted to: {}", exportPath);

                    auto loader = ((MaterialLoader*) (GetAssetManager().GetStorage<Material>().loader.get()));
                    int n = 0;
                    for(auto mesh : model->GetMeshes()) {
                        auto mat = mesh->GetMaterial();

                        log->info("Mat name {}", mat->GetName());
                        log->info("--> albedo {}", mat->GetDiffuseTexture().GetID());
                        Material mt = *mat.get();
                        loader->SaveMaterial(mt, exportPath + "/material-" + mat->GetName() + "#" + (std::to_string(n++))+".material");
                    }


                }else {
                    log->warn("No path selected, so materials won't be extracted!");
                }
            }


            // Right column: Preview
            ImGui::SameLine();
        }




        ImGui::End();
    }


    void UIManager::RenderGBufferDebug(std::shared_ptr<GBuffer> gbuffer)
    {
        ImGui::Begin("GBuffer Debug", &GetEditor().showGBufferDebug);

        const float previewSize = 400.0f;

        ImVec2 uv0 = ImVec2(0, 1);
        ImVec2 uv1 = ImVec2(1, 0);

        if(ImGui::CollapsingHeader("SSAO")) {
            ImGui::Indent();
            ImGui::Text("SSAO");
            ImGui::Image((ImTextureID) (intptr_t) GetWindow().GetSSAOBuffer()->ssaoTex,
                         ImVec2(previewSize, previewSize),
                         uv0, uv1);

            ImGui::Text("SSAO BLUR");
            ImGui::Image((ImTextureID) (intptr_t) GetWindow().GetSSAOBuffer()->blurTex,
                         ImVec2(previewSize, previewSize),
                         uv0, uv1);

            ImGui::Unindent();
        }

        if(ImGui::CollapsingHeader("BLOOM")) {
            ImGui::Indent();

            ImGui::SliderFloat("Threshold", &GetRenderSettings()->bloom_threshold, 0.1, 2.0);
            ImGui::SliderFloat("Knee", &GetRenderSettings()->bloom_knee, 0.1,0.5);

            auto br = GetRenderer().GetBloomRenderer();

            int i = 0;
            for(auto bm : br->GetBloomMips()) {
                ImGui::Text("MIP %d   ( %f x %f )", (++i), bm.size.x, bm.size.y);

                ImGui::Image((ImTextureID) (intptr_t) bm.fb.texture,
                             ImVec2(previewSize, previewSize),
                             uv0, uv1);
                }



            ImGui::Unindent();
        }

        ImGui::Text("Albedo");
        ImGui::Image((ImTextureID)(intptr_t)gbuffer->GetAlbedo(),
                     ImVec2(previewSize, previewSize),
                     uv0, uv1);

        ImGui::Text("Normal");
        ImGui::Image((ImTextureID)(intptr_t)gbuffer->GetNormal(),
                     ImVec2(previewSize, previewSize),
                     uv0, uv1);

        ImGui::Text("Material");
        ImGui::Image((ImTextureID)(intptr_t)gbuffer->GetMaterial(),
                     ImVec2(previewSize, previewSize),
                     uv0, uv1);

        ImGui::Text("Emissive");
        ImGui::Image((ImTextureID)(intptr_t)gbuffer->GetEmissive(),
                     ImVec2(previewSize, previewSize),
                     uv0, uv1);

        ImGui::Text("Depth");
        ImGui::Image((ImTextureID)(intptr_t)gbuffer->GetDepth(),
                     ImVec2(previewSize, previewSize),
                     uv0, uv1);



        ImGui::End();
    }

	bool UIManager::isOverSceneView() const
	{
		return m_overSceneView;
	}



    void UIManager::setLuaBindings() {

        // Input binding
        GetScriptManager().lua.new_usertype<UIManager>("UI",
                                                   "getSelectedEntity",
                                                   &UIManager::getSelectedEntity
        );


        // Global accessor
        GetScriptManager().lua.set_function("getUI", []() -> UIManager& { return Engine::GetUI(); });

    }



} // namespace Engine::UI

#include "assets/AssetManager.inl"