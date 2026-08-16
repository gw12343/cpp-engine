//
// Created by gabe on 8/24/25.
//

#include "SceneViewWindow.h"
#include "core/EngineData.h"
#include "core/Entity.h"
#include "core/Input.h"
#include "core/Window.h"


#include "rendering/ui/UIManager.h"
#include "rendering/ui/EditorSession.h"
#include "components/impl/TransformComponent.h"



#include "components/impl/EntityMetadataComponent.h"
#include "components/AllComponents.h"
#include "rendering/ui/IconsFontAwesome6.h"

#include <cstdio>
#include <cmath>

namespace Engine {

	namespace {
		void DrawViewportFpsOverlay(const ImVec2& viewportTopLeft)
		{
			const ImGuiIO& io  = ImGui::GetIO();
			const float    fps = io.Framerate;
			const float    ms  = (fps > 0.0f) ? (1000.0f / fps) : 0.0f;

			char buf[64];
			std::snprintf(buf, sizeof(buf), "%.0f FPS  (%.1f ms)", fps, ms);

			const ImVec2 textSize = ImGui::CalcTextSize(buf);
			const float  pad     = 6.0f;
			const ImVec2 pos     = ImVec2(viewportTopLeft.x + 8.0f, viewportTopLeft.y + 8.0f);
			const ImVec2 min     = ImVec2(pos.x - pad * 0.5f, pos.y - 2.0f);
			const ImVec2 max     = ImVec2(pos.x + textSize.x + pad * 0.5f, pos.y + textSize.y + 2.0f);

			ImDrawList* dl = ImGui::GetWindowDrawList();
			dl->AddRectFilled(min, max, IM_COL32(0, 0, 0, 150), 4.0f);
			dl->AddText(pos, IM_COL32(180, 255, 120, 255), buf);
		}

	} // namespace

	ImGuizmo::OPERATION SceneViewWindow::mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
	ImGuizmo::MODE      SceneViewWindow::mCurrentGizmoMode      = ImGuizmo::LOCAL;

	bool SceneViewWindow::DrawSceneViewWindow()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

		bool open = true;
		ImGui::Begin("Viewport", &open);

		ImVec2 sizeOut = ImGui::GetContentRegionAvail();

		auto& editor = UI::GetEditor();
		float width  = sizeOut.x;
		float height = sizeOut.y;
		if (editor.lockAspect && sizeOut.x > 1.0f && sizeOut.y > 1.0f) {
			const float target = editor.lockedAspect;
			if (sizeOut.x / sizeOut.y > target) {
				height = sizeOut.y;
				width  = height * target;
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (sizeOut.x - width) * 0.5f);
			}
			else {
				width  = sizeOut.x;
				height = width / target;
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (sizeOut.y - height) * 0.5f);
			}
		}

		ImVec2 topLeft = ImGui::GetCursorScreenPos();

		GLuint tex = Window::GetFramebuffer(Window::FramebufferID::GAME_OUT)->texture;

		ImGui::Image((ImTextureID) tex, ImVec2(width, height), ImVec2(0, 1), ImVec2(1, 0));

		GetWindow().UpdateViewportSize((int) width, (int) height, (int) topLeft.x, (int) topLeft.y);

		// FPS overlay (editor / play / paused — always on the viewport)
		DrawViewportFpsOverlay(topLeft);

		Entity* selectedEntity = &GetUI().m_selectedEntity;


		if (GetState() != PLAYING) {
			ImGuizmo::SetOrthographic(false);
			ImGuizmo::SetDrawlist(ImGui::GetCurrentWindow()->DrawList);
			ImGuizmo::SetRect(topLeft.x, topLeft.y, width, height);

			glm::mat4 view       = GetCamera().GetViewMatrix();
			glm::mat4 projection = GetCamera().GetProjectionMatrix();

			const bool canManipulate = *selectedEntity && GetCurrentSceneRegistry().valid(selectedEntity->GetENTTHandle()) &&
			                           selectedEntity->HasComponent<Components::Transform>() &&
			                           !editor.IsEntityLocked(*selectedEntity);

			if (*selectedEntity && GetCurrentSceneRegistry().valid(selectedEntity->GetENTTHandle()) &&
			    selectedEntity->HasComponent<Components::Transform>()) {
				auto& meta = selectedEntity->GetComponent<Components::EntityMetadata>();
				auto& tr   = selectedEntity->GetComponent<Components::Transform>();
				glm::mat4 model = tr.GetWorldMatrix();

				if (canManipulate) {
					const bool snapOn = editor.snapEnabled || GetInput().IsKeyPressed(GLFW_KEY_LEFT_CONTROL) ||
					                    GetInput().IsKeyPressed(GLFW_KEY_RIGHT_CONTROL) ||
					                    GetInput().IsKeyPressed(GLFW_KEY_LEFT_SUPER) ||
					                    GetInput().IsKeyPressed(GLFW_KEY_RIGHT_SUPER);

					float snapValues[3] = {editor.snapTranslate, editor.snapTranslate, editor.snapTranslate};
					if (mCurrentGizmoOperation == ImGuizmo::ROTATE) {
						snapValues[0] = snapValues[1] = snapValues[2] = editor.snapRotateDeg;
					}
					else if (mCurrentGizmoOperation == ImGuizmo::SCALE || mCurrentGizmoOperation == ImGuizmo::SCALEU ||
					         mCurrentGizmoOperation == ImGuizmo::BOUNDS) {
						snapValues[0] = snapValues[1] = snapValues[2] = editor.snapScale;
					}

					const bool changed = ImGuizmo::Manipulate(
					    glm::value_ptr(view),
					    glm::value_ptr(projection),
					    mCurrentGizmoOperation,
					    mCurrentGizmoMode,
					    glm::value_ptr(model),
					    nullptr,
					    snapOn ? snapValues : nullptr);

					if (changed) {
						glm::vec3 worldPos, worldScale;
						glm::quat worldRot;
						Components::Transform::ExtractTRS(model, worldPos, worldRot, worldScale);
						tr.SetWorldFromMatrix(model);

						if (meta.parentEntity.IsValid()) {
							auto parentEntity = GetCurrentScene()->Get(meta.parentEntity);
							if (parentEntity && parentEntity.HasComponent<Components::Transform>()) {
								auto& parentTr = parentEntity.GetComponent<Components::Transform>();
								tr.SetLocalFromWorld(parentTr.GetWorldMatrix(), worldPos, worldRot, worldScale);
							}
						}
						else {
							tr.SetLocalPosition(worldPos);
							tr.SetLocalRotation(worldRot);
							tr.SetLocalScale(worldScale);
						}

						tr.SyncWithPhysics(*selectedEntity);
						editor.MarkDirty();
					}
				}

				if (GetUI().isOverSceneView() && !ImGui::GetIO().WantTextInput && ImGui::IsKeyPressed(ImGuiKey_F, false)) {
					const glm::vec3 pos = tr.GetWorldPosition();
					GetCamera().SetPosition(pos - GetCamera().GetFront() * 6.0f);
				}
			}
		}
		ImGui::PopStyleVar();
		ImVec2 min   = ImGui::GetWindowPos(); // top-left corner in screen space
		ImVec2 max   = ImVec2(min.x + ImGui::GetWindowSize().x,
                            min.y + ImGui::GetWindowSize().y); // bottom-right corner
		ImVec2 mouse = ImGui::GetMousePos();

		bool b = (mouse.x >= min.x && mouse.x < max.x && mouse.y >= min.y && mouse.y < max.y);
		ImGui::End();

		// GPU readback is a full pipeline stall — only pick on click, never every frame.
		if (GetState() != PLAYING && GetInput().IsMousePositionInViewport() &&
		    !GetUI().m_inspectorRenderer->m_openPopup && GetInput().IsMouseClicked(0) && !ImGuizmo::IsOver() &&
		    !GetInput().IsKeyPressed(GLFW_KEY_LEFT_ALT) && !GetInput().IsKeyPressed(GLFW_KEY_RIGHT_ALT)) {
			Engine::Window::GetFramebuffer(Window::FramebufferID::MOUSE_PICKING)->Bind();
			glm::vec2 pos = GetInput().GetMousePositionInViewportScaledFlipped();

			GLfloat pixelData[3];
			glReadPixels(static_cast<GLint>(pos.x), static_cast<GLint>(pos.y), 1, 1, GL_RGB, GL_FLOAT, pixelData);

			uint32_t entityID = (static_cast<uint32_t>(pixelData[0] * 255.0f)) |
			                    (static_cast<uint32_t>(pixelData[1] * 255.0f) << 8) |
			                    (static_cast<uint32_t>(pixelData[2] * 255.0f) << 16);

			if (entityID != 0xFFFFFF) {
				*selectedEntity = Entity{static_cast<entt::entity>(entityID), GetCurrentScene()};
			}
			else {
				*selectedEntity = Entity();
			}

			Engine::Framebuffer::Unbind();
		}
		return b;
	}
} // namespace Engine