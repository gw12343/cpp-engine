#include "Camera.h"

#include "core/Input.h"

#include <algorithm>
#include <utils/Utils.h>

namespace Engine {

	Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch) : m_front(glm::vec3(0.0f, 0.0f, -1.0f)), m_movementSpeed(2.5f), m_mouseSensitivity(0.1f), m_fov(90.0f), m_nearPlane(0.1f), m_farPlane(750.0f)
	{
		m_position = position;
		m_worldUp  = up;
		m_yaw      = yaw;
		m_pitch    = pitch;

		UpdateCameraVectors();
	}

	ozz::math::Float4x4 Camera::view_proj() const
	{
		ENGINE_ASSERT(m_window, "Camera::view_proj: m_window is null");
		float aspect = m_window->GetTargetAspectRatio();
		ENGINE_VERIFY(aspect > 0.0f, "Camera::view_proj: aspect ratio is non-positive");
		return FromMatrix(GetProjectionMatrix(aspect)) * FromMatrix(GetViewMatrix());
	}

	glm::mat4 Camera::GetViewMatrix() const
	{
		return glm::lookAt(m_position, m_position + m_front, m_up);
	}

	glm::mat4 Camera::GetProjectionMatrix(float aspectRatio) const
	{
		ENGINE_ASSERT(aspectRatio > 0.0f, "Camera::GetProjectionMatrix: Invalid aspect ratio");
		return glm::perspective(glm::radians(m_fov), aspectRatio, m_nearPlane, m_farPlane);
	}

	void Camera::ProcessKeyboard(float deltaTime)
	{
		ENGINE_ASSERT(deltaTime >= 0.0f, "Camera::ProcessKeyboard: Negative delta time");
		float velocity = m_movementSpeed * deltaTime;

		if (Input::IsKeyPressed(GLFW_KEY_LEFT_CONTROL)) {
			velocity *= 20.0f;
		}

		if (Input::IsKeyPressed(GLFW_KEY_W)) m_position += m_front * velocity;
		if (Input::IsKeyPressed(GLFW_KEY_S)) m_position -= m_front * velocity;
		if (Input::IsKeyPressed(GLFW_KEY_A)) m_position -= m_right * velocity;
		if (Input::IsKeyPressed(GLFW_KEY_D)) m_position += m_right * velocity;
		if (Input::IsKeyPressed(GLFW_KEY_SPACE)) m_position += m_worldUp * velocity;
		if (Input::IsKeyPressed(GLFW_KEY_LEFT_SHIFT)) m_position -= m_worldUp * velocity;
	}

	void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch)
	{
		ENGINE_ASSERT(std::isfinite(xoffset) && std::isfinite(yoffset), "Camera::ProcessMouseMovement: Offset values are not finite");

		xoffset *= m_mouseSensitivity;
		yoffset *= m_mouseSensitivity;

		m_yaw += xoffset;
		m_pitch += yoffset;

		if (constrainPitch) {
			m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
		}

		UpdateCameraVectors();
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

} // namespace Engine