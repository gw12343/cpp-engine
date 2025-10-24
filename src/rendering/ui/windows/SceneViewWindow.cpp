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
			if (*selectedEntity && GetCurrentSceneRegistry().valid(selectedEntity->GetHandle())) {
				auto& meta = selectedEntity->GetComponent<Components::EntityMetadata>();


				if (selectedEntity->HasComponent<Components::Transform>()) {
					auto& tr = selectedEntity->GetComponent<Components::Transform>();

					ImGuizmo::SetRect(topLeft.x, topLeft.y, width, height);

					glm::mat4 view       = GetCamera().GetViewMatrix();
					glm::mat4 projection = GetCamera().GetProjectionMatrix();

					glm::mat4 model = tr.worldMatrix;

					ImGuizmo::SetDrawlist(ImGui::GetCurrentWindow()->DrawList);
					ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), mCurrentGizmoOperation, mCurrentGizmoMode, glm::value_ptr(model));

					if (ImGuizmo::IsUsingAny()) {
						glm::vec3 outTranslation, outScale, skew;
						glm::vec4 perspective;
						glm::quat outRotation;

						glm::decompose(model, outScale, outRotation, outTranslation, skew, perspective);

						tr.worldPosition = outTranslation;
						tr.worldRotation = outRotation;
						tr.worldScale    = outScale;


						//  Apply to local transform, respecting hierarchy
						if (meta.parentEntity.IsValid()) {
							// Child: convert world to local
							auto parentEntity = GetCurrentScene()->Get(meta.parentEntity);
							if (parentEntity && parentEntity.HasComponent<Components::Transform>()) {
								auto&     parentTr  = parentEntity.GetComponent<Components::Transform>();
								glm::mat4 parentInv = glm::inverse(parentTr.worldMatrix);

								glm::mat4 localMatrix = parentInv * model;

								glm::vec3 skewLocal;
								glm::vec4 perspLocal;
								glm::quat localRot;
								glm::vec3 localTrans, localScale;
								glm::decompose(localMatrix, localScale, localRot, localTrans, skewLocal, perspLocal);

								tr.localPosition = localTrans;
								tr.localRotation = localRot;
								tr.localScale    = localScale;
							}
						}
						else {
							// No hierarchy component: local equal to world
							tr.localPosition = tr.worldPosition;
							tr.localRotation = tr.worldRotation;
							tr.localScale    = tr.worldScale;
						}

						// Rebuild world matrix to stay consistent
						tr.worldMatrix = glm::translate(glm::mat4(1.0f), tr.worldPosition) * glm::toMat4(tr.worldRotation) * glm::scale(glm::mat4(1.0f), tr.worldScale);

						tr.SyncWithPhysics(*selectedEntity);
					}
				}
			}


			if (GetInput().IsKeyPressed(GLFW_KEY_LEFT_CONTROL) && GetInput().IsKeyPressedThisFrame(GLFW_KEY_D) && !GetInput().IsMousePressed(GLFW_MOUSE_BUTTON_RIGHT)) {
				if (*selectedEntity && GetCurrentScene()->GetRegistry()->valid(selectedEntity->GetHandle())) {
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