#include "UIManager.h"

#include "components/Components.h"
#include "core/EngineData.h"

#include "rendering/Renderer.h"
#include "rendering/ui/IconsFontAwesome6.h"

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

		m_audioIconTexture   = std::make_shared<Texture>();
		m_terrainIconTexture = std::make_shared<Texture>();
		m_audioIconTexture->LoadFromFile("resources/engine/speaker.png");
		m_terrainIconTexture->LoadFromFile("resources/engine/mountain.png");
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

		windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

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
		if (GetState() != PLAYING) {
			GetCamera().SaveEditorLocation();
			if (GetState() == EDITOR) {
				SCENE_LOADER::SerializeScene(GetSceneManager().GetActiveScene(), "scenes/scene1.json");
			}
			SetState(PLAYING);
			Get().manager->StartGame();
		}
	}

	void Pause()
	{
		if (GetState() == PLAYING) {
			GetCamera().LoadEditorLocation();
			SetState(PAUSED);
		}
	}

	void Stop()
	{
		// unload scene, load backup
		if (GetState() != EDITOR) {
			GetCamera().LoadEditorLocation();
			GetUI().m_selectedEntity = Entity();
			GetParticleManager().StopAllEffects();
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
			GetSceneManager().SetActiveScene(GetAssetManager().Load<Scene>("scenes/scene1.json"));
			SetState(EDITOR);
		}
	}


	float UIManager::RenderTopBar()
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();

		ImGui::SetNextWindowPos(viewport->Pos);
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
	if (mCurrentGizmoOperation == ImGuizmo::type) {                                                                                                                                                                                            \
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25, 0.25, 0.75, 1.0));                                                                                                                                                                 \
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35, 0.35, 0.85, 1.0));                                                                                                                                                          \
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.45, 0.45, 0.95, 1.0));                                                                                                                                                           \
	}                                                                                                                                                                                                                                          \
	bool Tool##type = ImGui::Button(name);                                                                                                                                                                                                     \
	if (mCurrentGizmoOperation == ImGuizmo::type) {                                                                                                                                                                                            \
		ImGui::PopStyleColor(3);                                                                                                                                                                                                               \
	}                                                                                                                                                                                                                                          \
	if (Tool##type) {                                                                                                                                                                                                                          \
		mCurrentGizmoOperation = ImGuizmo::type;                                                                                                                                                                                               \
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
			int         current     = (mCurrentGizmoMode == ImGuizmo::MODE::LOCAL ? 0 : 1);
			ImGui::SetNextItemWidth(80);
			if (ImGui::BeginCombo("##WorldMode", modeNames[current])) {
				for (int i = 0; i < 2; i++) {
					bool isSelected = (current == i);
					if (ImGui::Selectable(modeNames[i], isSelected)) current = i;

					if (isSelected) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			mCurrentGizmoMode = (current == 0 ? ImGuizmo::MODE::LOCAL : ImGuizmo::MODE::WORLD);


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
		m_selectedTheme = GetState() == EDITOR ? 0 : 2;
		GetRenderer().PreRender();
		float height = RenderTopBar();
		BeginDockspace(height);

		DrawSceneViewWindow();

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

		for (auto entity : view) {
			Entity e(entity, GetCurrentScene());
			auto&  metadata = e.GetComponent<Components::EntityMetadata>();

			// Create a selectable item for each entity
			bool isSelected = (m_selectedEntity == e);

			std::string title = metadata.name + "##" + std::to_string((int) entity);
			if (ImGui::Selectable(title.c_str(), isSelected)) {
				m_selectedEntity = e;
			}
		}
		if (ImGui::BeginPopupContextWindow()) {
			if (ImGui::MenuItem("Create Entity")) {
				// Action for menu item
				Entity entity    = Entity::Create("New Entity", GetCurrentScene());
				m_selectedEntity = entity;
			}

			ImGui::EndPopup();
		}


		ImGui::End();
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


} // namespace Engine::UI

#include "assets/AssetManager.inl"