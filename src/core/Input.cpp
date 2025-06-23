#include "Input.h"
#include "EngineData.h"

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include "Window.h"

#include "scripting/ScriptManager.h"

namespace Engine {


	void Input::onShutdown()
	{
	}


	void Input::onInit()
	{
		// glfwSetScrollCallback(GetWindow().GetNativeWindow(), ScrollCallback);

		// Prime the last-mouse position so GetMouseDelta is zero on first frame
		double x, y;
		glfwGetCursorPos(GetWindow().GetNativeWindow(), &x, &y);
		m_lastMousePosition = glm::vec2((float) x, (float) y);

		SetCursorMode(GLFW_CURSOR_NORMAL);
	}


	void Input::onUpdate(float dt)
	{
		// Update mouse position
		// m_scrollDelta       = 0;
		m_lastMousePosition = m_mousePosition;
		double x, y;
		glfwGetCursorPos(GetWindow().GetNativeWindow(), &x, &y);
		m_mousePosition = glm::vec2(x, y);

		// Update key states - copy current to previous, then update current
		m_prevKeyStates = m_keyStates;

		// Clear and rebuild current key states
		m_keyStates.clear();
		for (const auto& pair : m_prevKeyStates) {
			int key          = pair.first;
			m_keyStates[key] = IsKeyPressed(key);
		}
	}

	bool Input::IsMousePressed(int btn)
	{
		return glfwGetMouseButton(GetWindow().GetNativeWindow(), btn);
	}

	bool Input::IsKeyPressed(int key)
	{
		return glfwGetKey(GetWindow().GetNativeWindow(), key) == GLFW_PRESS;
	}

	bool Input::IsKeyReleased(int key)
	{
		return glfwGetKey(GetWindow().GetNativeWindow(), key) == GLFW_RELEASE;
	}

	[[maybe_unused]] glm::vec2 Input::GetMousePosition()
	{
		return m_mousePosition;
	}

	glm::vec2 Input::GetMouseDelta() const
	{
		return glm::vec2(m_mousePosition.x, m_lastMousePosition.y) - glm::vec2(m_lastMousePosition.x, m_mousePosition.y); // m_mousePosition - m_lastMousePosition;
	}

	//	float Input::GetMouseScrollDelta()
	//	{
	//		float f       = m_scrollDelta;
	//		m_scrollDelta = 0;
	//		return f;
	//	}

	[[maybe_unused]] void Input::SetMousePosition(const glm::vec2& pos)
	{
		glfwSetCursorPos(GetWindow().GetNativeWindow(), pos.x, pos.y);
	}

	void Input::SetCursorMode(int mode)
	{
		glfwSetInputMode(GetWindow().GetNativeWindow(), GLFW_CURSOR, mode);
	}

	int Input::GetCursorMode()
	{
		return glfwGetInputMode(GetWindow().GetNativeWindow(), GLFW_CURSOR);
	}

	//	[[maybe_unused]] void Input::MouseCallback(GLFWwindow* /*wnd*/, double /*x*/, double /*y*/)
	//	{
	//	}
	//
	//	void Input::ScrollCallback(GLFWwindow* /*wnd*/, double /*xoff*/, double yoff)
	//	{
	//		m_scrollDelta = static_cast<float>(yoff);
	//	}

	bool Input::IsKeyPressedThisFrame(int key)
	{
		// Make sure we're tracking this key
		if (m_keyStates.find(key) == m_keyStates.end()) {
			m_keyStates[key] = IsKeyPressed(key);
		}

		// Get current state
		bool currentState = m_keyStates[key];

		// Get previous state (false if not tracked in previous frame)
		bool previousState = false;
		auto it            = m_prevKeyStates.find(key);
		if (it != m_prevKeyStates.end()) {
			previousState = it->second;
		}

		// Return true only if currently pressed but wasn't pressed last frame
		return currentState && !previousState;
	}


	void Input::setLuaBindings()
	{
		// glm::vec2 usertype (only if not already bound)
		GetScriptManager().lua.new_usertype<glm::vec2>("vec2", sol::constructors<glm::vec2(), glm::vec2(float, float)>(), "x", &glm::vec2::x, "y", &glm::vec2::y);

		// Input binding
		GetScriptManager().lua.new_usertype<Input>("Input",
		                                           "isKeyPressed",
		                                           &Input::IsKeyPressed,
		                                           "isKeyReleased",
		                                           &Input::IsKeyReleased,
		                                           "isKeyPressedThisFrame",
		                                           &Input::IsKeyPressedThisFrame,

		                                           "isMousePressed",
		                                           &Input::IsMousePressed,
		                                           "getMousePosition",
		                                           &Input::GetMousePosition,
		                                           "getMouseDelta",
		                                           &Input::GetMouseDelta,
		                                           "setMousePosition",
		                                           &Input::SetMousePosition,
		                                           "setCursorMode",
		                                           &Input::SetCursorMode,
		                                           "getCursorMode",
		                                           &Input::GetCursorMode);

		GetScriptManager().lua["KEY_W"]           = GLFW_KEY_W;
		GetScriptManager().lua["KEY_SPACE"]       = GLFW_KEY_SPACE;
		GetScriptManager().lua["MOUSE_LEFT"]      = GLFW_MOUSE_BUTTON_LEFT;
		GetScriptManager().lua["MOUSE_RIGHT"]     = GLFW_MOUSE_BUTTON_RIGHT;
		GetScriptManager().lua["CURSOR_NORMAL"]   = GLFW_CURSOR_NORMAL;
		GetScriptManager().lua["CURSOR_DISABLED"] = GLFW_CURSOR_DISABLED;
		GetScriptManager().lua["CURSOR_HIDDEN"]   = GLFW_CURSOR_HIDDEN;

		// Global accessor
		GetScriptManager().lua.set_function("getInput", []() -> Input& {
			return Engine::GetInput(); // You implement this
		});
	}


} // namespace Engine