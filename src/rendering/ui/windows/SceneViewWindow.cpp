//
// Created by gabe on 8/24/25.
//

#include "SceneViewWindow.h"
#include "core/EngineData.h"
#include "core/Entity.h"
#include "core/Input.h"
#include "core/Window.h"


#include "rendering/ui/UIManager.h"
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

		// Extract TRS from a column-major affine matrix without glm::decompose
		// (glm::decompose param order is easy to swap and can flip rotations).
		void ExtractTRS(const glm::mat4& m, glm::vec3& translation, glm::quat& rotation, glm::vec3& scale)
		{
			translation = glm::vec3(m[3]);

			const float sx = glm::length(glm::vec3(m[0]));
			const float sy = glm::length(glm::vec3(m[1]));
			const float sz = glm::length(glm::vec3(m[2]));
			scale          = glm::vec3(sx, sy, sz);

			glm::mat3 rot(1.0f);
			const float eps = 1e-8f;
			rot[0]          = (sx > eps) ? (glm::vec3(m[0]) / sx) : glm::vec3(1.f, 0.f, 0.f);
			rot[1]          = (sy > eps) ? (glm::vec3(m[1]) / sy) : glm::vec3(0.f, 1.f, 0.f);
			rot[2]          = (sz > eps) ? (glm::vec3(m[2]) / sz) : glm::vec3(0.f, 0.f, 1.f);

			// Handle reflection (negative scale) so quat_cast stays valid
			if (glm::determinant(rot) < 0.0f) {
				scale.x = -scale.x;
				rot[0]  = -rot[0];
			}

			rotation = glm::normalize(glm::quat_cast(rot));
		}

		glm::mat4 ComposeTRS(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale)
		{
			return glm::translate(glm::mat4(1.0f), translation) * glm::mat4_cast(rotation) * glm::scale(glm::mat4(1.0f), scale);
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

		float aspect = (float) sizeOut.y / (float) sizeOut.x;

		float width, height;
		float offsetX, offsetY;

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

		GLuint tex = Window::GetFramebuffer(Window::FramebufferID::GAME_OUT)->texture;

		ImGui::Image((ImTextureID) tex, ImVec2(width, height), ImVec2(0, 1), ImVec2(1, 0));

		GetWindow().UpdateViewportSize((int) width, (int) height, (int) topLeft.x, (int) topLeft.y);

		// FPS overlay (editor / play / paused — always on the viewport)
		DrawViewportFpsOverlay(topLeft);

		Entity* selectedEntity = &GetUI().m_selectedEntity;


		if (GetState() != PLAYING) {
			if (*selectedEntity && GetCurrentSceneRegistry().valid(selectedEntity->GetENTTHandle())) {
				auto& meta = selectedEntity->GetComponent<Components::EntityMetadata>();


				if (selectedEntity->HasComponent<Components::Transform>()) {
					auto& tr = selectedEntity->GetComponent<Components::Transform>();

					ImGuizmo::SetOrthographic(false);
					ImGuizmo::SetDrawlist(ImGui::GetCurrentWindow()->DrawList);
					ImGuizmo::SetRect(topLeft.x, topLeft.y, width, height);

					// Copies — ImGuizmo may write view; keep camera matrices intact
					glm::mat4 view       = GetCamera().GetViewMatrix();
					glm::mat4 projection = GetCamera().GetProjectionMatrix();
					glm::mat4 model      = tr.GetWorldMatrix();

					// Hold Ctrl (or Super) to snap translate / rotate / scale.
					const bool snapHeld = GetInput().IsKeyPressed(GLFW_KEY_LEFT_CONTROL) ||
					                      GetInput().IsKeyPressed(GLFW_KEY_RIGHT_CONTROL) ||
					                      GetInput().IsKeyPressed(GLFW_KEY_LEFT_SUPER) ||
					                      GetInput().IsKeyPressed(GLFW_KEY_RIGHT_SUPER);

					// ImGuizmo: translate uses xyz snap; rotate uses degrees in .x; scale uses .x
					float snapValues[3] = {0.5f, 0.5f, 0.5f};
					if (mCurrentGizmoOperation == ImGuizmo::ROTATE) {
						snapValues[0] = snapValues[1] = snapValues[2] = 15.0f; // degrees
					}
					else if (mCurrentGizmoOperation == ImGuizmo::SCALE ||
					         mCurrentGizmoOperation == ImGuizmo::SCALEU ||
					         mCurrentGizmoOperation == ImGuizmo::BOUNDS) {
						snapValues[0] = snapValues[1] = snapValues[2] = 0.1f;
					}

					// Manipulate returns true only when the matrix changes this frame.
					// Do NOT re-decompose every IsUsing frame — that causes rotation drift.
					const bool changed = ImGuizmo::Manipulate(
					    glm::value_ptr(view),
					    glm::value_ptr(projection),
					    mCurrentGizmoOperation,
					    mCurrentGizmoMode,
					    glm::value_ptr(model),
					    nullptr,
					    snapHeld ? snapValues : nullptr);

					if (changed) {
						glm::vec3 worldPos, worldScale;
						glm::quat worldRot;
						ExtractTRS(model, worldPos, worldRot, worldScale);

						// Keep TRS + world matrix consistent with the gizmo matrix
						tr.SetWorldPosition(worldPos);
						tr.SetWorldRotation(worldRot);
						tr.SetWorldScale(worldScale);
						tr.SetWorldMatrix(ComposeTRS(worldPos, worldRot, worldScale));

						// Local = parent^-1 * world (hierarchy)
						if (meta.parentEntity.IsValid()) {
							auto parentEntity = GetCurrentScene()->Get(meta.parentEntity);
							if (parentEntity && parentEntity.HasComponent<Components::Transform>()) {
								auto&     parentTr    = parentEntity.GetComponent<Components::Transform>();
								glm::mat4 localMatrix = glm::inverse(parentTr.GetWorldMatrix()) * model;

								glm::vec3 localPos, localScale;
								glm::quat localRot;
								ExtractTRS(localMatrix, localPos, localRot, localScale);

								tr.SetLocalPosition(localPos);
								tr.SetLocalRotation(localRot);
								tr.SetLocalScale(localScale);
							}
						}
						else {
							tr.SetLocalPosition(worldPos);
							tr.SetLocalRotation(worldRot);
							tr.SetLocalScale(worldScale);
						}

						tr.SyncWithPhysics(*selectedEntity);
					}
				}
			}


			if (GetInput().IsKeyPressed(GLFW_KEY_LEFT_CONTROL) && GetInput().IsKeyPressedThisFrame(GLFW_KEY_D) && !GetInput().IsMousePressed(GLFW_MOUSE_BUTTON_RIGHT)) {
				if (*selectedEntity && GetCurrentScene()->GetRegistry()->valid(selectedEntity->GetENTTHandle())) {
					GetDefaultLogger()->warn("DUPLICATING");

					std::string newName = selectedEntity->GetComponent<Components::EntityMetadata>().name;
					if (!newName.rfind("Copy of ", 0) == 0) {
						newName = "Copy of " + newName;
					}
					Entity copy = Entity::Create(newName, selectedEntity->m_scene);

					// TODO COPY CONSTRUCTORS FOR COMPONENTS WITH DYNAMICALLY ALLOCATED MEMORY!!!!!!!
#define X(type, name, fancy)                                                                                                                                                                                                                   \
	if (selectedEntity->HasComponent<type>()) {                                                                                                                                                                                                \
		copy.AddComponent<type>(selectedEntity->GetComponent<type>());                                                                                                                                                                         \
		GetDefaultLogger()->warn("adding cmp: {}", fancy);                                                                                                                                                                                     \
	}
					COMPONENT_LIST
#undef X

					GetUI().m_selectedEntity = copy;
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
		    !GetUI().m_inspectorRenderer->m_openPopup && GetInput().IsMouseClicked(0) && !ImGuizmo::IsOver()) {
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