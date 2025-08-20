#include "UIManager.h"

#include "components/Components.h"
#include "components/impl/AnimationComponent.h"
#include "spdlog/spdlog.h"
#include "glm/glm.hpp"
#include "core/EngineData.h"
#include "animation/AnimationManager.h"

#include "rendering/Renderer.h"
#include "rendering/ui/IconsFontAwesome6.h"

#include "components/impl/LuaScriptComponent.h"
#include "components/impl/EntityMetadataComponent.h"
#include "components/impl/SkeletonComponent.h"
#include "components/impl/RigidBodyComponent.h"
#include "components/impl/AudioSourceComponent.h"
#include "components/impl/ModelRendererComponent.h"
#include "components/impl/AnimationPoseComponent.h"
#include "components/impl/AnimationWorkerComponent.h"
#include "components/impl/SkinnedMeshComponent.h"
#include "components/impl/ParticleSystemComponent.h"
#include "components/impl/ShadowCasterComponent.h"
#include "imgui_internal.h"
#include "components/impl/TerrainRendererComponent.h"


#include "rendering/ui/Themes.h"
#include "imguizmo/ImGuizmo.h"

#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/matrix_decompose.inl"


namespace Engine::UI {


	void UIManager::onShutdown()
	{
	}


	void UIManager::onInit()
	{
		SetThemeColors(0);


		m_uiAssetRenderer = std::make_unique<AssetUIRenderer>();

		audioIconTexture   = std::make_shared<Texture>();
		terrainIconTexture = std::make_shared<Texture>();
		audioIconTexture->LoadFromFile("resources/engine/speaker.png");
		terrainIconTexture->LoadFromFile("resources/engine/mountain.png");
	}

	int selectedTheme = 0;

	void UIManager::DrawMenuBar()
	{
		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("Themes")) {
				if (ImGui::MenuItem("Bess Dark", nullptr, selectedTheme == 0)) selectedTheme = 0;
				if (ImGui::MenuItem("Catpuccin Mocha", nullptr, selectedTheme == 1)) selectedTheme = 1;
				if (ImGui::MenuItem("Modern Dark", nullptr, selectedTheme == 2)) selectedTheme = 2;
				if (ImGui::MenuItem("Dark Theme", nullptr, selectedTheme == 3)) selectedTheme = 3;
				if (ImGui::MenuItem("Fluent UI", nullptr, selectedTheme == 4)) selectedTheme = 4;
				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}
	}


	void UIManager::BeginDockspace()
	{
		// 1. Draw top menu bar *before* dockspace
		// DrawTopBar();

		// 2. Apply theme colors
		SetThemeColors(selectedTheme);

		// 3. Setup full-screen window for dockspace
		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

		windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		bool dockspaceOpen = true;
		ImGui::Begin("Dockspace", &dockspaceOpen, windowFlags);
		ImGui::PopStyleVar(3); // Pop all three style vars

		// 4. Create the dockspace
		ImGuiID dockspaceID = ImGui::GetID("Dockspace");
		ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
	}

	void UIManager::EndDockspace()
	{
		ImGui::End();
	}

	static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);
	static ImGuizmo::MODE      mCurrentGizmoMode(ImGuizmo::LOCAL);

	void UIManager::RenderTopBar()
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();

		// Place the top bar exactly at the top of the viewport
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 32.0f)); // fixed height
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

			if (ImGui::Button(ICON_FA_PLAY "##play")) { /* play */
			}
			ImGui::SameLine();

			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);                        // Disable input
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.25f); // Dim look

			if (ImGui::Button(ICON_FA_PAUSE "##pause")) { /* pause */
			}

			ImGui::PopStyleVar();
			ImGui::PopItemFlag();

			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_STOP "##stop")) { /* stop */
			}
		}
		ImGui::End();

		ImGui::PopStyleVar(3);
	}

	void UIManager::onUpdate(float dt)
	{
		GetRenderer().PreRender();
		RenderTopBar();


		BeginDockspace();

		RenderSceneView(Engine::Window::GetFramebuffer(Window::FramebufferID::GAME_OUT)->texture);
		RenderHierarchyWindow();
		RenderInspectorWindow();
		RenderAnimationWindow();

		m_uiAssetRenderer->RenderAssetWindow();
		RenderAudioDebugUI();

		// Display pause overlay when physics is disabled
		if (GetPhysics().isPhysicsPaused) {
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


	void UIManager::RenderInspectorWindow()
	{
		ImGui::Begin("Inspector");

		if (m_selectedEntity) {
			// Display entity name at the top
			auto& metadata = m_selectedEntity.GetComponent<Components::EntityMetadata>();
			ImGui::Text("Selected: %s", metadata.name.c_str());
			ImGui::SameLine();
			ImVec4 myColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // bluish

			ImGui::PushStyleColor(ImGuiCol_Button, myColor);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(myColor.x * 1.1f, myColor.y * 1.1f, myColor.z * 1.1f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(myColor.x * 0.9f, myColor.y * 0.9f, myColor.z * 0.9f, 1.0f));

			if (ImGui::Button(ICON_FA_TRASH_CAN "", ImVec2(100, 40))) {
				m_selectedEntity.Destroy();
				if (GetCurrentSceneRegistry().valid(m_selectedEntity.GetHandle())) GetCurrentSceneRegistry().destroy(m_selectedEntity.GetHandle());
				ImGui::PopStyleColor(3);
				ImGui::End();
				m_selectedEntity = Entity();
				return;
			}


			ImGui::PopStyleColor(3);

			ImGui::Separator();

			// Render each component in the inspector
			if (m_selectedEntity.HasComponent<Components::EntityMetadata>()) {
				if (ImGui::CollapsingHeader(ICON_FA_ID_CARD " Entity Metadata", ImGuiTreeNodeFlags_DefaultOpen)) {
					m_selectedEntity.GetComponent<Components::EntityMetadata>().RenderInspector(m_selectedEntity);
				}
			}


			if (m_selectedEntity.HasComponent<Components::Transform>()) {
				if (ImGui::CollapsingHeader(ICON_FA_MAXIMIZE " Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
					m_selectedEntity.GetComponent<Components::Transform>().RenderInspector(m_selectedEntity);
				}
			}

			if (m_selectedEntity.HasComponent<Components::ModelRenderer>()) {
				if (ImGui::CollapsingHeader(ICON_FA_CUBE " Model Renderer")) {
					m_selectedEntity.GetComponent<Components::ModelRenderer>().RenderInspector(m_selectedEntity);
				}
			}

			if (m_selectedEntity.HasComponent<Components::RigidBodyComponent>()) {
				if (ImGui::CollapsingHeader(ICON_FA_CUBES_STACKED " Rigid Body")) {
					m_selectedEntity.GetComponent<Components::RigidBodyComponent>().RenderInspector(m_selectedEntity);
				}
			}

			if (m_selectedEntity.HasComponent<Components::AudioSource>()) {
				if (ImGui::CollapsingHeader(ICON_FA_VOLUME_HIGH " Audio Source")) {
					auto& audioSource = m_selectedEntity.GetComponent<Components::AudioSource>();
					audioSource.RenderInspector(m_selectedEntity);

					// Handle the Play button functionality here since we have access to the sound manager
					//					if (!audioSource.isPlaying && ImGui::Button("Play")) {
					//						if (audioSource.source && !audioSource.soundName.empty()) {
					//							audioSource.Play(GetSoundManager());
					//							spdlog::error("Playing sound");
					//						}
					//					}
				}
			}

			if (m_selectedEntity.HasComponent<Components::SkeletonComponent>()) {
				if (ImGui::CollapsingHeader("Skeleton")) {
					m_selectedEntity.GetComponent<Components::SkeletonComponent>().RenderInspector(m_selectedEntity);
				}
			}

			if (m_selectedEntity.HasComponent<Components::AnimationComponent>()) {
				if (ImGui::CollapsingHeader("Animation")) {
					m_selectedEntity.GetComponent<Components::AnimationComponent>().RenderInspector(m_selectedEntity);
				}
			}

			if (m_selectedEntity.HasComponent<Components::AnimationPoseComponent>()) {
				if (ImGui::CollapsingHeader("Animation Pose")) {
					m_selectedEntity.GetComponent<Components::AnimationPoseComponent>().RenderInspector(m_selectedEntity);
				}
			}

			if (m_selectedEntity.HasComponent<Components::AnimationWorkerComponent>()) {
				if (ImGui::CollapsingHeader("Animation Context")) {
					m_selectedEntity.GetComponent<Components::AnimationWorkerComponent>().RenderInspector(m_selectedEntity);
				}
			}

			if (m_selectedEntity.HasComponent<Components::SkinnedMeshComponent>()) {
				if (ImGui::CollapsingHeader("Skinned Mesh")) {
					m_selectedEntity.GetComponent<Components::SkinnedMeshComponent>().RenderInspector(m_selectedEntity);
				}
			}
			if (m_selectedEntity.HasComponent<Components::ParticleSystem>()) {
				if (ImGui::CollapsingHeader(ICON_FA_STAR_HALF_STROKE "Particle System")) {
					m_selectedEntity.GetComponent<Components::ParticleSystem>().RenderInspector(m_selectedEntity);
				}
			}
			if (m_selectedEntity.HasComponent<Components::ShadowCaster>()) {
				if (ImGui::CollapsingHeader(ICON_FA_MOON " Shadow Caster")) {
					m_selectedEntity.GetComponent<Components::ShadowCaster>().RenderInspector(m_selectedEntity);
				}
			}
			if (m_selectedEntity.HasComponent<Components::LuaScript>()) {
				if (ImGui::CollapsingHeader(ICON_FA_SCROLL " Script")) {
					m_selectedEntity.GetComponent<Components::LuaScript>().RenderInspector(m_selectedEntity);
				}
			}
			if (m_selectedEntity.HasComponent<Components::TerrainRenderer>()) {
				if (ImGui::CollapsingHeader(ICON_FA_MAP " Terrain Renderer")) {
					m_selectedEntity.GetComponent<Components::TerrainRenderer>().RenderInspector(m_selectedEntity);
				}
			}
		}
		else {
			ImGui::Text("No entity selected");
		}

		ImGui::End();
	}

	void UIManager::RenderAnimationWindow()
	{
		ImGui::Begin("Animation");

		if (ImGui::CollapsingHeader("Animation")) {
			//						auto& animManager = GetAnimationManager();
			//
			//
			//						auto& skeleton = animManager.GetSkeleton();
			//						auto& meshes   = animManager.GetMeshes();
			//
			//						ImGui::LabelText("Animated Joints", "%d animated joints", skeleton.num_joints());
			//
			//						int influences = 0;
			//						for (const auto& mesh : meshes) {
			//							influences = ozz::math::Max(influences, mesh.max_influences_count());
			//						}
			//						ImGui::LabelText("Influences", "%d influences (max)", influences);
			//
			//						int vertices = 0;
			//						for (const auto& mesh : meshes) {
			//							vertices += mesh.vertex_count();
			//						}
			//
			//						ImGui::LabelText("Vertices", "%.1fK vertices", vertices / 1000.f);
			//
			//						int indices = 0;
			//						for (const auto& mesh : meshes) {
			//							indices += mesh.triangle_index_count();
			//						}
			//
			//						ImGui::LabelText("Triangles", "%.1fK triangles", indices / 3000.f);
		}

		if (ImGui::CollapsingHeader("Rendering Options")) {
			auto& animManager    = GetAnimationManager();
			auto& draw_skeleton  = animManager.GetDrawSkeleton();
			auto& draw_mesh      = animManager.GetDrawMesh();
			auto& render_options = animManager.GetRenderOptions();

			ImGui::Checkbox("Show Skeleton", &draw_skeleton);
			ImGui::Checkbox("Show Mesh", &draw_mesh);
			ImGui::Separator();

			ImGui::Checkbox("Show triangles", &render_options.triangles);
			ImGui::Checkbox("Show texture", &render_options.texture);
			ImGui::Checkbox("Show vertices", &render_options.vertices);
			ImGui::Checkbox("Show normals", &render_options.normals);
			ImGui::Checkbox("Show tangents", &render_options.tangents);
			ImGui::Checkbox("Show binormals", &render_options.binormals);
			ImGui::Checkbox("Show colors", &render_options.colors);
			ImGui::Checkbox("Wireframe", &render_options.wireframe);
			ImGui::Checkbox("Skip skinning", &render_options.skip_skinning);
		}

		ImGui::SliderFloat("fov", &GetCamera().m_fov, 45, 120);

		ImGui::End();
	}

	void UIManager::RenderAudioDebugUI()
	{
		ImGui::Begin("Audio Debug");
		auto      audioView = GetCurrentSceneRegistry().view<Components::EntityMetadata, Components::Transform, Components::AudioSource>();
		glm::vec3 cameraPos = GetCamera().GetPosition();

		for (auto [entity, metadata, transform, audio] : audioView.each()) {
			if (audio.source) {
				float distance = glm::distance(cameraPos, transform.position);
				ImGui::Text("Entity: %s", metadata.name.c_str());
				ImGui::Text("Distance: %.2f units", distance);

				// Add sliders to adjust audio parameters
				bool paramsChanged = false;

				paramsChanged |= ImGui::SliderFloat("Volume", &audio.volume, 0.0f, 1.0f);
				if (paramsChanged) {
					audio.source->SetGain(audio.volume);
				}

				paramsChanged = false;
				paramsChanged |= ImGui::SliderFloat("Reference Distance", &audio.referenceDistance, 0.1f, 20.0f);
				paramsChanged |= ImGui::SliderFloat("Max Distance", &audio.maxDistance, 10.0f, 100.0f);
				paramsChanged |= ImGui::SliderFloat("Rolloff Factor", &audio.rolloffFactor, 0.1f, 5.0f);

				if (paramsChanged) {
					audio.source->ConfigureAttenuation(audio.referenceDistance, audio.maxDistance, audio.rolloffFactor);
				}

				// Calculate linear attenuation for display (matching OpenAL's AL_LINEAR_DISTANCE_CLAMPED)
				float attenuation = 1.0f;

				if (distance <= audio.referenceDistance) {
					// Within reference distance - full volume
					attenuation = 1.0f;
				}
				else if (distance >= audio.maxDistance) {
					// Beyond max distance - silent
					attenuation = 0.0f;
				}
				else {
					// Linear interpolation between reference and max distance
					attenuation = 1.0f - ((distance - audio.referenceDistance) / (audio.maxDistance - audio.referenceDistance));

					// Apply rolloff factor
					attenuation = pow(attenuation, audio.rolloffFactor);
				}

				ImGui::Text("Estimated Attenuation: %.3f", attenuation);
				ImGui::Text("Estimated Volume: %.3f", audio.volume * attenuation);

				// Add a visual representation of the attenuation
				ImGui::Text("Attenuation:");
				ImGui::SameLine();
				ImGui::ProgressBar(attenuation, ImVec2(100, 10));

				ImGui::Separator();
			}
		}
		ImGui::End();
	}

	void UIManager::RenderPauseOverlay()
	{
		ImGuiIO& io = ImGui::GetIO();
		// ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 10, 10), ImGuiCond_Always, ImVec2(1.0f, 0.0f));

		ImGui::SetNextWindowPos(ImVec2(GetWindow().targetX + GetWindow().targetWidth - 10, GetWindow().targetY + 10), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
		ImGui::SetNextWindowBgAlpha(0.35f);
		ImGui::Begin("PauseOverlay", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove);
		ImGui::Text("PHYSICS PAUSED");
		ImGui::Text("Mouse  (%f, %f)", GetInput().GetMousePosition().x, GetInput().GetMousePosition().y);
		glm::vec2 pos = GetInput().GetMousePositionInViewportScaledFlipped();
		ImGui::Text("MouseW (%f, %f)", pos.x, pos.y);
		ImGui::Text("m %d", GetInput().IsMousePositionInViewport());


		if (GetInput().IsMousePositionInViewport()) {
			Engine::Window::GetFramebuffer(Window::FramebufferID::MOUSE_PICKING)->Bind();


			GLfloat pixelData[3]; // Use GLfloat for normalized color values
			glReadPixels(pos.x, pos.y, 1, 1, GL_RGB, GL_FLOAT, pixelData);

			uint32_t entityID = (static_cast<uint32_t>(pixelData[0] * 255.0f)) | (static_cast<uint32_t>(pixelData[1] * 255.0f) << 8) | (static_cast<uint32_t>(pixelData[2] * 255.0f) << 16);

			ImGui::Text("color %d", entityID);
			if (GetInput().IsMousePressed(0) && !ImGuizmo::IsOver()) {
				if (entityID != 0xFFFFFF) {
					m_selectedEntity = (Entity){static_cast<entt::entity>(entityID), GetCurrentScene()};
				}
				else {
					m_selectedEntity = Entity();
				}
			}


			Engine::Framebuffer::Unbind();
		}


		ImGui::Text("Press P to resume");
		ImGui::End();
	}


	void UIManager::RenderSceneView(GLuint texId)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

		bool open = true;
		ImGui::Begin("Viewport", &open);

		ImVec2 sizeOut = ImGui::GetContentRegionAvail();
		bool   focus   = ImGui::IsWindowFocused();

		float aspect = (float) sizeOut.y / (float) sizeOut.x;

		float width, height;
		float offsetX = 0.0f, offsetY = 0.0f;

		if (sizeOut.y / sizeOut.x > aspect) {
			width   = sizeOut.x;
			height  = width * aspect;
			offsetY = (sizeOut.y - height) / 2.0f;
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offsetY);
		}
		else {
			height  = sizeOut.y;
			width   = height / aspect;
			offsetX = (sizeOut.x - width) / 2.0f;
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
		}

		ImVec2 topLeft = ImGui::GetCursorScreenPos();

		GLuint tex = texId;
		ImGui::Image((ImTextureID) tex, ImVec2(width, height), ImVec2(0, 1), ImVec2(1, 0));

		GetWindow().UpdateViewportSize(width, height, topLeft.x, topLeft.y);

		if (m_selectedEntity && GetCurrentSceneRegistry().valid(m_selectedEntity.GetHandle())) {
			if (m_selectedEntity.HasComponent<Components::Transform>()) {
				Components::Transform& transform = m_selectedEntity.GetComponent<Components::Transform>();

				ImGuizmo::SetRect(topLeft.x, topLeft.y, width, height);

				glm::mat4 view       = GetCamera().GetViewMatrix();
				glm::mat4 projection = GetCamera().GetProjectionMatrix();

				glm::mat4 model = transform.GetMatrix();
				ImGuizmo::SetDrawlist(ImGui::GetCurrentWindow()->DrawList);
				ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), mCurrentGizmoOperation, mCurrentGizmoMode, glm::value_ptr(model));
				if (ImGuizmo::IsUsingAny()) {
					glm::vec3 outTrans, outScale, notUsed1;
					glm::vec4 notUsed2;
					glm::quat newRotation;

					glm::decompose(model, outScale, newRotation, outTrans, notUsed1, notUsed2);

					transform.position = outTrans;
					transform.rotation = newRotation;
					transform.scale    = outScale;
					transform.SyncWithPhysics(m_selectedEntity);
				}
			}
		}
		ImGui::PopStyleVar();
		ImGui::End();
	}


} // namespace Engine::UI
