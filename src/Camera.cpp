#include "Camera.h"

#include "core/Input.h"

#include <algorithm>
#include <utils/Utils.h>

namespace Engine {
	Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
	    : m_front(glm::vec3(0.0f, 0.0f, -1.0f)), m_movementSpeed(2.5f), m_mouseSensitivity(0.1f), m_fov(70.0f), m_nearPlane(0.1f), m_farPlane(1000.0f)
	{ // Increased from 100.0f to 1000.0f
		m_position = position;
		m_worldUp  = up;
		m_yaw      = yaw;
		m_pitch    = pitch;
		UpdateCameraVectors();
	}


	ozz::math::Float4x4 Camera::view_proj() const
	{
		return FromMatrix(GetProjectionMatrix((float) m_window->GetWidth() / (float) m_window->GetHeight())) * FromMatrix(GetViewMatrix());
	}

	glm::mat4 Camera::GetViewMatrix() const
	{
		return glm::lookAt(m_position, m_position + m_front, m_up);
	}

	glm::mat4 Camera::GetProjectionMatrix(float aspectRatio) const
	{
		return glm::perspective(glm::radians(m_fov), aspectRatio, m_nearPlane, m_farPlane);
	}

	void Camera::ProcessKeyboard(float deltaTime)
	{
		float velocity = m_movementSpeed * deltaTime;

		// Apply speed boost when Control is held
		if (Input::IsKeyPressed(GLFW_KEY_LEFT_CONTROL)) {
			velocity *= 2.0f; // Double the speed when Control is held
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
		xoffset *= m_mouseSensitivity;
		yoffset *= m_mouseSensitivity;

		m_yaw += xoffset;
		m_pitch += yoffset;

		// Make sure that when pitch is out of bounds, screen doesn't get flipped
		if (constrainPitch) {
			m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
		}

		// Update front, right and up Vectors using the updated Euler angles
		UpdateCameraVectors();
	}

	void Camera::ProcessMouseScroll(float yoffset)
	{
		m_fov -= yoffset;
		m_fov = std::clamp(m_fov, 1.0f, 70.0f);
	}

	void Camera::UpdateCameraVectors()
	{
		// Calculate the new front vector
		glm::vec3 front;
		front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
		front.y = sin(glm::radians(m_pitch));
		front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
		m_front = glm::normalize(front);

		// Also re-calculate the right and up vector
		m_right = glm::normalize(glm::cross(m_front, m_worldUp));
		m_up    = glm::normalize(glm::cross(m_right, m_front));
	}


} // namespace Engine