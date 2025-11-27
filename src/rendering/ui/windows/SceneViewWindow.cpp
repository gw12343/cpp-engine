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


#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/matrix_decompose.inl"
#include "components/impl/EntityMetadataComponent.h"
#include "components/AllComponents.h"
#include "rendering/ui/IconsFontAwesome6.h"

#ifndef GAME_BUILD
#include "core/EditorCommandStack.h"
#include "core/commands/TransformCommand.h"
#include "core/commands/CloneEntityCommand.h"
#include "core/commands/DeleteEntityCommand.h"
#include "core/EntityClipboard.h"
#endif
namespace Engine {

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

		Entity* selectedEntity = &GetUI().m_selectedEntity;


		if (GetState() != PLAYING) {
			if (*selectedEntity && GetCurrentSceneRegistry().valid(selectedEntity->GetENTTHandle())) {
				auto& meta = selectedEntity->GetComponent<Components::EntityMetadata>();


				if (selectedEntity->HasComponent<Components::Transform>()) {
				auto& tr = selectedEntity->GetComponent<Components::Transform>();

				ImGuizmo::SetRect(topLeft.x, topLeft.y, width, height);

				glm::mat4 view       = GetCamera().GetViewMatrix();
				glm::mat4 projection = GetCamera().GetProjectionMatrix();

				glm::mat4 model = tr.GetWorldMatrix();

#ifndef GAME_BUILD
				// Track ImGuizmo manipulation for undo/redo
				static bool wasUsing = false;
				static glm::vec3 initialPosition;
				static glm::quat initialRotation;
				static glm::vec3 initialScale;
				static EntityHandle manipulatedEntity;

				bool isUsing = ImGuizmo::IsUsing();

				// Detect start of manipulation
				if (isUsing && !wasUsing) {
					initialPosition = tr.GetLocalPosition();
					initialRotation = tr.GetLocalRotation();
					initialScale = tr.GetLocalScale();
					manipulatedEntity = selectedEntity->GetEntityHandle();
				}
#endif

				ImGuizmo::SetDrawlist(ImGui::GetCurrentWindow()->DrawList);
				ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), mCurrentGizmoOperation, mCurrentGizmoMode, glm::value_ptr(model));

				if (ImGuizmo::IsUsingAny()) {
					glm::vec3 outTranslation, outScale, skew;
					glm::vec4 perspective;
					glm::quat outRotation;

					glm::decompose(model, outScale, outRotation, outTranslation, skew, perspective);

					tr.SetWorldPosition(outTranslation);
					tr.SetWorldRotation(outRotation);
					tr.SetWorldScale(outScale);


					//  Apply to local transform, respecting hierarchy
					if (meta.parentEntity.IsValid()) {
						// Child: convert world to local
						auto parentEntity = GetCurrentScene()->Get(meta.parentEntity);
						if (parentEntity && parentEntity.HasComponent<Components::Transform>()) {
							auto&     parentTr  = parentEntity.GetComponent<Components::Transform>();
							glm::mat4 parentInv = glm::inverse(parentTr.GetWorldMatrix());

							glm::mat4 localMatrix = parentInv * model;

							glm::vec3 skewLocal;
							glm::vec4 perspLocal;
							glm::quat localRot;
							glm::vec3 localTrans, localScale;
							glm::decompose(localMatrix, localScale, localRot, localTrans, skewLocal, perspLocal);

							tr.SetLocalPosition(localTrans);
							tr.SetLocalRotation(localRot);
							tr.SetLocalScale(localScale);
						}
					}
					else {
						// No hierarchy component: local equal to world
						tr.SetLocalPosition(tr.GetWorldPosition());
						tr.SetLocalRotation(tr.GetWorldRotation());
						tr.SetLocalScale(tr.GetWorldScale());
					}

					// Rebuild world matrix to stay consistent
					tr.SetWorldMatrix(glm::translate(glm::mat4(1.0f), tr.GetWorldPosition()) * glm::toMat4(tr.GetWorldRotation()) * glm::scale(glm::mat4(1.0f), tr.GetWorldScale()));

					tr.SyncWithPhysics(*selectedEntity);
				}

#ifndef GAME_BUILD
				// Detect end of manipulation - create undo command
				if (!isUsing && wasUsing) {
					glm::vec3 finalPosition = tr.GetLocalPosition();
					glm::quat finalRotation = tr.GetLocalRotation();
					glm::vec3 finalScale = tr.GetLocalScale();

					// Only create command if transform actually changed
					if (glm::distance(initialPosition, finalPosition) > 0.001f ||
					    glm::distance(initialScale, finalScale) > 0.001f ||
					    glm::abs(glm::dot(initialRotation, finalRotation)) < 0.999f) {
						auto command = std::make_unique<TransformCommand>(
						    manipulatedEntity,
						    initialPosition, initialRotation, initialScale,
						    finalPosition, finalRotation, finalScale
						);
						// Note: Execute() was already performed by ImGuizmo manipulation
						GetCommandStack().AddCommandWithoutExecute(std::move(command));
					}
				}

				wasUsing = isUsing;
#endif
			}
		}
			if (GetInput().IsKeyPressed(GLFW_KEY_LEFT_CONTROL) && GetInput().IsKeyPressedThisFrame(GLFW_KEY_D) && !GetInput().IsMousePressed(GLFW_MOUSE_BUTTON_RIGHT)) {
			if (*selectedEntity && GetCurrentScene()->GetRegistry()->valid(selectedEntity->GetENTTHandle())) {
#ifndef GAME_BUILD
				auto cmd = std::make_unique<CloneEntityCommand>(selectedEntity->GetEntityHandle());
				GetCommandStack().ExecuteCommand(std::move(cmd));
				// Note: We can't easily get the cloned entity handle from the command after it's moved,
				// so selection stays on original entity. User can click to select the clone.
#else
				// Fallback for game build (though this code path shouldn't be hit in GAME_BUILD)
				GetDefaultLogger()->warn("Clone not available in game build");
#endif
			}
		}	}
		ImGui::PopStyleVar();
		ImVec2 min   = ImGui::GetWindowPos(); // top-left corner in screen space
		ImVec2 max   = ImVec2(min.x + ImGui::GetWindowSize().x,
                            min.y + ImGui::GetWindowSize().y); // bottom-right corner
		ImVec2 mouse = ImGui::GetMousePos();

		bool b = (mouse.x >= min.x && mouse.x < max.x && mouse.y >= min.y && mouse.y < max.y);
		ImGui::End();

		if (GetState() != PLAYING && GetInput().IsMousePositionInViewport()) {
			Engine::Window::GetFramebuffer(Window::FramebufferID::MOUSE_PICKING)->Bind();
			glm::vec2 pos = GetInput().GetMousePositionInViewportScaledFlipped();

			GLfloat pixelData[3];
			glReadPixels(static_cast<GLint>(pos.x), static_cast<GLint>(pos.y), 1, 1, GL_RGB, GL_FLOAT, pixelData);

			uint32_t entityID = (static_cast<uint32_t>(pixelData[0] * 255.0f)) | (static_cast<uint32_t>(pixelData[1] * 255.0f) << 8) | (static_cast<uint32_t>(pixelData[2] * 255.0f) << 16);


			if (!GetUI().m_inspectorRenderer->m_openPopup && GetInput().IsMouseClicked(0) && !ImGuizmo::IsOver()) {
				if (entityID != 0xFFFFFF) {
					*selectedEntity = (Entity){static_cast<entt::entity>(entityID), GetCurrentScene()};
				}
				else {
					*selectedEntity = Entity();
				}
			}


			Engine::Framebuffer::Unbind();
		}
		return b;
	}
} // namespace Engine