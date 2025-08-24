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

namespace Engine {

	void DrawSceneViewWindow()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

		bool open = true;
		ImGui::Begin("Viewport", &open);

		ImVec2 sizeOut = ImGui::GetContentRegionAvail();

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

		GLuint tex = GetWindow().GetFramebuffer(Window::FramebufferID::GAME_OUT)->texture;

		ImGui::Image((ImTextureID) tex, ImVec2(width, height), ImVec2(0, 1), ImVec2(1, 0));

		GetWindow().UpdateViewportSize((int) width, (int) height, (int) topLeft.x, (int) topLeft.y);

		Entity* selectedEntity = &GetUI().m_selectedEntity;

		if (GetState() != PLAYING) {
			if (*selectedEntity && GetCurrentSceneRegistry().valid(selectedEntity->GetHandle())) {
				if (selectedEntity->HasComponent<Components::Transform>()) {
					auto& transform = selectedEntity->GetComponent<Components::Transform>();

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
						transform.SyncWithPhysics(*selectedEntity);
					}
				}
			}
		}
		ImGui::PopStyleVar();
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
	}
} // namespace Engine