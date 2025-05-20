#include "Input.h"

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
namespace Engine {
	GLFWwindow*                   Input::s_window            = nullptr;
	glm::vec2                     Input::s_mousePosition     = glm::vec2(0.0f);
	glm::vec2                     Input::s_lastMousePosition = glm::vec2(0.0f);
	float                         Input::s_scrollDelta       = 0.0f;
	std::unordered_map<int, bool> Input::s_keyStates;
	std::unordered_map<int, bool> Input::s_prevKeyStates;

	void Input::Initialize(GLFWwindow* window)
	{
		s_window = window;
		glfwSetScrollCallback(window, ScrollCallback);

		// Prime the last-mouse position so GetMouseDelta is zero on first frame
		double x, y;
		glfwGetCursorPos(s_window, &x, &y);
		s_lastMousePosition = glm::vec2((float) x, (float) y);
	}

	void Input::Update()
	{
		// Update mouse position
		s_scrollDelta       = 0;
		s_lastMousePosition = s_mousePosition;
		double x, y;
		glfwGetCursorPos(s_window, &x, &y);
		s_mousePosition = glm::vec2(x, y);

		// Update key states - copy current to previous, then update current
		s_prevKeyStates = s_keyStates;

		// Clear and rebuild current key states
		s_keyStates.clear();
		for (const auto& pair : s_prevKeyStates) {
			int key          = pair.first;
			s_keyStates[key] = IsKeyPressed(key);
		}
	}

	bool Input::IsMousePressed(int btn)
	{
		return glfwGetMouseButton(s_window, btn);
	}

	bool Input::IsKeyPressed(int key)
	{
		return glfwGetKey(s_window, key) == GLFW_PRESS;
	}

	bool Input::IsKeyReleased(int key)
	{
		return glfwGetKey(s_window, key) == GLFW_RELEASE;
	}

	glm::vec2 Input::GetMousePosition()
	{
		return s_mousePosition;
	}

	glm::vec2 Input::GetMouseDelta()
	{
		return glm::vec2(s_mousePosition.x, s_lastMousePosition.y) -
		       glm::vec2(s_lastMousePosition.x, s_mousePosition.y); // s_mousePosition - s_lastMousePosition;
	}

	float Input::GetMouseScrollDelta()
	{
		float f       = s_scrollDelta;
		s_scrollDelta = 0;
		return f;
	}

	void Input::SetMousePosition(const glm::vec2& pos)
	{
		glfwSetCursorPos(s_window, pos.x, pos.y);
	}

	void Input::SetCursorMode(int mode)
	{
		glfwSetInputMode(s_window, GLFW_CURSOR, mode);
	}

	int Input::GetCursorMode()
	{
		return glfwGetInputMode(s_window, GLFW_CURSOR);
	}

	void Input::MouseCallback(GLFWwindow* /*wnd*/, double /*x*/, double /*y*/)
	{
	}

	void Input::ScrollCallback(GLFWwindow* /*wnd*/, double /*xoff*/, double yoff)
	{
		s_scrollDelta = static_cast<float>(yoff);
	}

	bool Input::IsKeyPressedThisFrame(int key)
	{
		// Make sure we're tracking this key
		if (s_keyStates.find(key) == s_keyStates.end()) {
			s_keyStates[key] = IsKeyPressed(key);
		}

		// Get current state
		bool currentState = s_keyStates[key];

		// Get previous state (false if not tracked in previous frame)
		bool previousState = false;
		auto it            = s_prevKeyStates.find(key);
		if (it != s_prevKeyStates.end()) {
			previousState = it->second;
		}

		// Return true only if currently pressed but wasn't pressed last frame
		return currentState && !previousState;
	}
} // namespace Engine