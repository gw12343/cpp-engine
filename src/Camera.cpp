#include "Camera.h"

#include "core/Input.h"
#include "core/EngineData.h"

#include <algorithm>
#include <utils/Utils.h>
#include <tracy/Tracy.hpp>
#include "core/Window.h"

#include "scripting/ScriptManager.h"
#include "imgui.h"

namespace Engine {

	Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch) : m_front(glm::vec3(0.0f, 0.0f, -1.0f)), m_movementSpeed(2.5f), m_mouseSensitivity(0.1f), m_fov(90.0f), m_nearPlane(0.1f), m_farPlane(1500.0f)
	{
		m_position = position;
		m_worldUp  = up;
		m_yaw      = yaw;
		m_pitch    = pitch;
	}


	void Camera::onInit()
	{
		UpdateCameraVectors();
	}
	void Camera::onUpdate(float dt)
	{
		ZoneScoped;
		if (GetState() != PLAYING) {
			// Handle camera movement based on right mouse button state
			if (GetInput().IsMousePressed(GLFW_MOUSE_BUTTON_RIGHT)) {
				// Only capture cursor if it's not already captured
				if (GetInput().GetCursorMode() != GLFW_CURSOR_DISABLED) {
					GetInput().SetCursorMode(GLFW_CURSOR_DISABLED);
					ImGuiIO& io = ImGui::GetIO();
					io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
				}

				// Process camera movement with keyboard
				GetCamera().ProcessKeyboard(dt);

				// Process mouse movement for camera rotation
				glm::vec2 mouseDelta = GetInput().GetMouseDelta();

				float xoffset = mouseDelta.x;
				float yoffset = mouseDelta.y;

				xoffset *= m_mouseSensitivity;
				yoffset *= m_mouseSensitivity;

				m_yaw += xoffset;
				m_pitch += yoffset;

				m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
			}
			else {
				// Release cursor when right mouse button is released
				if (GetInput().GetCursorMode() == GLFW_CURSOR_DISABLED) {
					GetInput().SetCursorMode(GLFW_CURSOR_NORMAL);
				}
				ImGuiIO& io = ImGui::GetIO();
				io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
			}
		}


		UpdateCameraVectors();

		float aspect = GetWindow().GetTargetAspectRatio();
		ENGINE_VERIFY(aspect > 0.0f, "Camera::view_proj: aspect ratio is non-positive");

		m_proj = glm::perspective(glm::radians(m_fov), aspect, m_nearPlane, m_farPlane);

		m_view = glm::lookAt(m_position, m_position + m_front, m_up);

		m_viewProj = FromMatrix(GetProjectionMatrix()) * FromMatrix(GetViewMatrix());
	}
	void Camera::onShutdown()
	{
	}


	ozz::math::Float4x4 Camera::view_proj() const
	{
		return m_viewProj;
	}

	glm::mat4 Camera::GetViewMatrix() const
	{
		return m_view;
	}

	glm::mat4 Camera::GetProjectionMatrix() const
	{
		return m_proj;
	}

	void Camera::ProcessKeyboard(float deltaTime)
	{
		ENGINE_ASSERT(deltaTime >= 0.0f, "Camera::ProcessKeyboard: Negative delta time");
		float velocity = m_movementSpeed * deltaTime;

		if (GetInput().IsKeyPressed(GLFW_KEY_LEFT_CONTROL)) {
			velocity *= 20.0f;
		}

		if (GetInput().IsKeyPressed(GLFW_KEY_W)) m_position += m_front * velocity;
		if (GetInput().IsKeyPressed(GLFW_KEY_S)) m_position -= m_front * velocity;
		if (GetInput().IsKeyPressed(GLFW_KEY_A)) m_position -= m_right * velocity;
		if (GetInput().IsKeyPressed(GLFW_KEY_D)) m_position += m_right * velocity;
		if (GetInput().IsKeyPressed(GLFW_KEY_SPACE)) m_position += m_worldUp * velocity;
		if (GetInput().IsKeyPressed(GLFW_KEY_LEFT_SHIFT)) m_position -= m_worldUp * velocity;
	}


	void Camera::ProcessMouseScroll(float yoffset)
	{
		ENGINE_ASSERT(std::isfinite(yoffset), "Camera::ProcessMouseScroll: Scroll offset is not finite");
		m_fov -= yoffset;
		m_fov = std::clamp(m_fov, 1.0f, 70.0f);
	}

	void Camera::UpdateCameraVectors()
	{
		// Sanity checks on input values
		ENGINE_VERIFY(std::isfinite(m_yaw) && std::isfinite(m_pitch), "Camera::UpdateCameraVectors: Invalid yaw or pitch");

		glm::vec3 front;
		front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
		front.y = sin(glm::radians(m_pitch));
		front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
		m_front = glm::normalize(front);

		ENGINE_VERIFY(glm::length(m_front) > 0.0f, "Camera::UpdateCameraVectors: Front vector has zero length");

		m_right = glm::normalize(glm::cross(m_front, m_worldUp));
		m_up    = glm::normalize(glm::cross(m_right, m_front));
	}
	void Camera::setLuaBindings()
	{
		// Camera binding
		GetScriptManager().lua.new_usertype<Camera>("Camera",
		                                            // Read-only getters
		                                            "getPosition",
		                                            &Camera::GetPosition,
		                                            "getFront",
		                                            &Camera::GetFront,

		                                            "setPosition",
		                                            &Camera::SetPosition,

		                                            // Public fields
		                                            "fov",
		                                            &Camera::m_fov,
		                                            "movementSpeed",
		                                            &Camera::m_movementSpeed,
		                                            "mouseSensitivity",
		                                            &Camera::m_mouseSensitivity,
		                                            "nearPlane",
		                                            &Camera::m_nearPlane,
		                                            "farPlane",
		                                            &Camera::m_farPlane,
		                                            "yaw",
		                                            &Camera::m_yaw,
		                                            "pitch",
		                                            &Camera::m_pitch);

		// Global getter
		GetScriptManager().lua.set_function("getCamera", []() -> Camera& { return Engine::GetCamera(); });
	}


} // namespace Engine