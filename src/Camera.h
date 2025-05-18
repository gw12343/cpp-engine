#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// include ozz mat4
#include "ozz/base/platform.h"
#include "ozz/base/maths/simd_math.h"
#include "core/Window.h"

class Camera {
public:
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f),
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
           float yaw = -90.0f,
           float pitch = 0.0f);
    
    void SetWindow(const Window* window) { m_window = window; }

    glm::mat4 GetViewMatrix() const;
    ozz::math::Float4x4 view_proj() const;
    glm::mat4 GetProjectionMatrix(float aspectRatio) const;
    void ProcessKeyboard(float deltaTime);
    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
    void ProcessMouseScroll(float yoffset);

    // Getters
    glm::vec3 GetPosition() const { return m_position; }
    glm::vec3 GetFront() const { return m_front; }

private:
    void UpdateCameraVectors();

    // Camera Attributes
    glm::vec3 m_position;
    glm::vec3 m_front;
    glm::vec3 m_up;
    glm::vec3 m_right;
    glm::vec3 m_worldUp;

    // Euler Angles
    float m_yaw;
    float m_pitch;

    // Camera options
    float m_movementSpeed;
    float m_mouseSensitivity;

    // Projection parameters
    float m_fov;
    float m_nearPlane;
    float m_farPlane;

    const Window* m_window;
}; 
