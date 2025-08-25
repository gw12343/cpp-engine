#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "ozz/base/maths/simd_math.h"
#include "ozz/base/platform.h"
#include "core/module/Module.h"

namespace Engine {

	class Camera : public Module {
	  public:
		explicit Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = -90.0f, float pitch = 0.0f);

		void                      onInit() override;
		void                      onUpdate(float dt) override;
		void                      onGameStart() override {}
		void                      onShutdown() override;
		[[nodiscard]] std::string name() const override { return "CameraModule"; };
		void                      setLuaBindings() override;


		void ProcessMouseScroll(float yoffset);
		void ProcessKeyboard(float deltaTime);

		[[nodiscard]] ozz::math::Float4x4 view_proj() const;
		[[nodiscard]] glm::mat4           GetViewMatrix() const;
		[[nodiscard]] glm::mat4           GetProjectionMatrix() const;

		// Getters
		[[nodiscard]] glm::vec3 GetPosition() const { return m_position; }
		[[nodiscard]] glm::vec3 GetFront() const { return m_front; }

		void SetPosition(glm::vec3 position) { m_position = position; }
		void SaveEditorLocation()
		{
			m_editorSavePos   = m_position;
			m_editorSaveRot.x = m_pitch;
			m_editorSaveRot.y = m_yaw;
		}

		void LoadEditorLocation()
		{
			m_position = m_editorSavePos;
			m_pitch    = m_editorSaveRot.x;
			m_yaw      = m_editorSaveRot.y;
			UpdateCameraVectors();
		}
		float m_fov;

		// Camera options
		float m_movementSpeed;
		float m_mouseSensitivity;

		float m_nearPlane;
		float m_farPlane;

		// Euler Angles
		float m_yaw;
		float m_pitch;

		glm::vec3 m_editorSavePos;
		glm::vec3 m_editorSaveRot;


	  private:
		void UpdateCameraVectors();

		glm::mat4           m_proj{};
		glm::mat4           m_view{};
		ozz::math::Float4x4 m_viewProj{};

		// Camera Attributes
		glm::vec3 m_position{};
		glm::vec3 m_front;
		glm::vec3 m_up{};
		glm::vec3 m_right{};
		glm::vec3 m_worldUp{};
	};
} // namespace Engine