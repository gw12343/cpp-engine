#include "UIManager.h"

#include "components/Components.h"
#include "core/EngineData.h"

#include "rendering/Renderer.h"
#include "rendering/ui/IconsFontAwesome6.h"
#include "rendering/ui/windows/SceneViewWindow.h"

#include "components/impl/EntityMetadataComponent.h"
#include "components/impl/TerrainRendererComponent.h"
#include "rendering/particles/ParticleManager.h"

#include "rendering/ui/Themes.h"

#include "assets/impl/JSONSceneLoader.h"

#include "physics/PhysicsManager.h"
#include "core/Input.h"

#include "core/SceneManager.h"
#include "core/module/ModuleManager.h"

#include "windows/ConsoleWindow.h"
#include "windows/AudioDebugWindow.h"
#include "windows/SceneViewWindow.h"
#include "windows/AnimationWindow.h"
#include "utils/Builder.h"
#include "components/impl/ModelRendererComponent.h"
#include "rendering/particles/Particle.h"
#include "components/impl/ParticleSystemComponent.h"

#include <nfd.h>
#include <string>
#include <iostream>
#include <tracy/Tracy.hpp>

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
		SetThemeColors(0);

		m_uiAssetRenderer   = std::make_unique<AssetUIRenderer>();
		m_materialEditor    = std::make_unique<MaterialEditor>();
		m_inspectorRenderer = std::make_unique<InspectorRenderer>();

		m_audioIconTexture     = std::make_shared<Texture>();
		m_terrainIconTexture   = std::make_shared<Texture>();
		m_animationIconTexture = std::make_shared<Texture>();
#ifndef GAME_BUILD
		m_audioIconTexture->LoadFromFile("resources/engine/speaker.png");
		m_terrainIconTexture->LoadFromFile("resources/engine/mountain.png");
		m_animationIconTexture->LoadFromFile("resources/engine/animation.png");

		efsw::WatchID id = m_uiAssetRenderer->fw.addWatch("resources", &m_uiAssetRenderer->listener, true);
		m_uiAssetRenderer->fw.watch();
#endif
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
		if (GetState() != PLAYING) {
			GetCamera().SaveEditorLocation();
			if (GetState() == EDITOR) {
				SCENE_LOADER::SerializeScene(GetSceneManager().GetActiveScene(), "scenes/scene1.json");
			}
			SetState(PLAYING);
			Get().manager->StartGame();
		}
#endif
	}

	void Pause()
	{
#ifndef GAME_BUILD
		if (GetState() == PLAYING) {
			GetCamera().LoadEditorLocation();
			SetState(PAUSED);
		}
#endif
	}

	void Stop()
	{
#ifndef GAME_BUILD

		// unload scene, load backup
		if (GetState() != EDITOR) {
			GetCamera().LoadEditorLocation();
			GetUI().m_selectedEntity = Entity();
			GetParticleManager().ResetInternalManager();
			{
				//  clear scene
				auto& physics = GetPhysics();

				BodyIDVector outBodies;
				physics.GetPhysicsSystem()->GetBodies(outBodies);

				for (auto body : outBodies) {
					if (physics.GetPhysicsSystem()->GetBodyInterface().IsAdded(body)) {
						physics.GetPhysicsSystem()->GetBodyInterface().RemoveBody(body);
					}
				}
			}
			GetAssetManager().Unload<Scene>(GetSceneManager().GetActiveScene());
			SetState(EDITOR);
			GetSceneManager().SetActiveScene(GetAssetManager().Load<Scene>("scenes/scene1.json"));
		}
#endif
	}


	float UIManager::RenderMainMenuBar()
	{
		float height = 0.0f;
		ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0, 0, 0, 0));
		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("File")) {
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
				static bool showDemo = false;
				if (ImGui::MenuItem("ImGui Demo", nullptr, showDemo)) showDemo = !showDemo;
				if (showDemo) ImGui::ShowDemoWindow(&showDemo);
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
		}

		float barHeight = ImGui::GetWindowHeight();
		ImGui::End();
		ImGui::PopStyleVar(3);

		return barHeight;
	}

	bool consoleOpen = true;

	void UIManager::onUpdate(float dt)
	{
		ZoneScoped;
		m_selectedTheme = GetState() == EDITOR ? 2 : 0;
		GetRenderer().PreRender();
		float h      = RenderMainMenuBar();
		float height = RenderTopBar(h) + h;
		BeginDockspace(height);

		m_overSceneView = SceneViewWindow::DrawSceneViewWindow();
		RenderHierarchyWindow();
		m_inspectorRenderer->RenderInspectorWindow(&m_selectedEntity);
		m_materialEditor->RenderMaterialEditor(m_selectedMaterial);

		DrawAnimationWindow();
		DrawAudioDebugWindow();
		DrawConsoleWindow(Logger::getImGuiSink(), &consoleOpen);

		m_uiAssetRenderer->RenderAssetWindow();
		// RenderAudioDebugUI();

		// Display pause overlay when physics is disabled
		if (GetState() == PAUSED) {
			RenderPauseOverlay();
		}

		EndDockspace();
	}

	void UIManager::RenderHierarchyWindow()
	{
		ImGui::Begin("Hierarchy");

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
				RenderEntityTreeNode(e);
			}
		}

		// Create an invisible button that fills the remaining space to accept drops
		ImVec2 contentRegionAvail = ImGui::GetContentRegionAvail();
		if (contentRegionAvail.y > 0) {
			ImGui::InvisibleButton("##HierarchyDropZone", contentRegionAvail);

			// Handle drop onto window background to unparent
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
				ImGui::EndDragDropTarget();
			}
		}

		if (ImGui::BeginPopupContextWindow("HierarchyContext", ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight)) {
			ImGui::Text("Add Entity");
			ImGui::Separator();
			if (ImGui::MenuItem("Empty Entity")) {
				Entity entity    = Entity::Create("New Entity", GetCurrentScene());
				m_selectedEntity = entity;
			}

			// Get position in front of camera
			glm::vec3 position = GetCamera().GetPosition();
			glm::vec3 forward  = GetCamera().GetFront();
			glm::vec3 spawnPos = position + glm::vec3(forward.x * 3, forward.y * 3, forward.z * 3);

			if (ImGui::MenuItem("Add Model")) {
				Entity                        entity = Entity::Create("New Model", GetCurrentScene());
				AssetHandle<Rendering::Model> model;

				if (!GetAssetManager().GetStorage<Rendering::Model>().guidToAsset.empty()) {
					model = AssetHandle<Rendering::Model>(GetAssetManager().GetStorage<Rendering::Model>().guidToAsset.begin()->first);
				}

				entity.AddComponent<Components::Transform>(spawnPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
				entity.AddComponent<Components::ModelRenderer>(model);
				m_selectedEntity = entity;
			}
			if (ImGui::MenuItem("Add Particle System")) {
				Entity                entity = Entity::Create("New Particle System", GetCurrentScene());
				AssetHandle<Particle> particle;

				if (!GetAssetManager().GetStorage<Particle>().guidToAsset.empty()) {
					particle = AssetHandle<Particle>(GetAssetManager().GetStorage<Particle>().guidToAsset.begin()->first);
				}

				entity.AddComponent<Components::Transform>(spawnPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
				entity.AddComponent<Components::ParticleSystem>(particle);

				m_selectedEntity = entity;
			}

			ImGui::EndPopup();
		}

		ImGui::End();
		if (changeParent) {
			_newChild.SetParent(_newParent);
		}
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
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
		if (isSelected) {
			flags |= ImGuiTreeNodeFlags_Selected;
		}

		// If entity has no children, make it a leaf node
		if (metadata.children.empty()) {
			flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
		}

		// Push ID using GUID to keep tree state stable when name changes
		ImGui::PushID(guid.c_str());

		// Render the tree node with just the name (no ID suffix in display)
		bool nodeOpen = ImGui::TreeNodeEx(metadata.name.c_str(), flags);

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
			ImGui::Text("Entity: %s", guid.c_str());

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
			ImGui::EndDragDropTarget();
		}


		// Handle selection - only when released and not dragging
		if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && ImGui::IsMouseReleased(ImGuiMouseButton_Left) && !isDragging) {
			m_selectedEntity = entity;
		}

		// Recursively render children
		if (nodeOpen && !metadata.children.empty()) {
			for (auto& childHandle : metadata.children) {
				auto childEntity = GetCurrentScene()->Get(childHandle);
				if (childEntity) {
					RenderEntityTreeNode(childEntity);
				}
			}
			ImGui::TreePop();
		}

		// Pop the ID
		ImGui::PopID();

		if (changeParent) {
			_newChild.SetParent(_newParent);
		}
	}


	void UIManager::RenderPauseOverlay()
	{
		ImGui::SetNextWindowPos(ImVec2(GetWindow().targetX + GetWindow().targetWidth - 10, GetWindow().targetY + 10), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
		ImGui::SetNextWindowBgAlpha(0.35f);
		ImGui::Begin("PauseOverlay", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove);
		ImGui::Text("GAME PAUSED");
		ImGui::Text("Press PLAY to resume");
		ImGui::End();
	}

	bool UIManager::isOverSceneView() const
	{
		return m_overSceneView;
	}


} // namespace Engine::UI

#include "assets/AssetManager.inl"