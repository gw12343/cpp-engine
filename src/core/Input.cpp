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

		m_gameCursorMode = GLFW_CURSOR_NORMAL;
		SetCursorMode(GLFW_CURSOR_NORMAL);
	}


	void Input::onUpdate(float dt)
	{
		if (GetState() == PLAYING) {
			if (IsKeyPressed(GLFW_KEY_ESCAPE)) {
				m_gameCursorMode = GLFW_CURSOR_NORMAL;
			}
			glfwSetInputMode(GetWindow().GetNativeWindow(), GLFW_CURSOR, m_gameCursorMode);
		}
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

	bool Input::IsMousePositionInViewport() const
	{
		glm::vec2 mousePosWindow = GetMousePositionInViewport();
		auto&     window         = GetWindow();
		return mousePosWindow.x >= 0 && mousePosWindow.y >= 0 && mousePosWindow.x < window.targetWidth && mousePosWindow.y < window.targetHeight;
	}

	glm::vec2 Input::GetMousePositionInViewportScaledFlipped() const
	{
		glm::vec2 mousePosWindow = GetMousePositionInViewport();
		auto&     window         = GetWindow();

		return {mousePosWindow.x * (float) window.GetWidth() / (float) window.targetWidth, window.GetHeight() - mousePosWindow.y * (float) window.GetHeight() / (float) window.targetHeight};
	}

	glm::vec2 Input::GetMousePositionInViewport() const
	{
		auto& window = GetWindow();

		return {m_mousePosition.x - window.targetX, m_mousePosition.y - window.targetY};
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

	void Input::SetCursorModeGame(int mode)
	{
		m_gameCursorMode = mode;
	}

	int Input::GetCursorModeGame()
	{
		return m_gameCursorMode;
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
		// GetScriptManager().lua.new_usertype<glm::vec2>("vec2", sol::constructors<glm::vec2(), glm::vec2(float, float)>(), "x", &glm::vec2::x, "y", &glm::vec2::y);

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
		                                           &Input::SetCursorModeGame,
		                                           "getCursorMode",
		                                           &Input::GetCursorModeGame);

		GetScriptManager().lua["KEY_SPACE"]         = GLFW_KEY_SPACE;
		GetScriptManager().lua["KEY_APOSTROPHE"]    = GLFW_KEY_APOSTROPHE;
		GetScriptManager().lua["KEY_COMMA"]         = GLFW_KEY_COMMA;
		GetScriptManager().lua["KEY_MINUS"]         = GLFW_KEY_MINUS;
		GetScriptManager().lua["KEY_PERIOD"]        = GLFW_KEY_PERIOD;
		GetScriptManager().lua["KEY_SLASH"]         = GLFW_KEY_SLASH;
		GetScriptManager().lua["KEY_0"]             = GLFW_KEY_0;
		GetScriptManager().lua["KEY_1"]             = GLFW_KEY_1;
		GetScriptManager().lua["KEY_2"]             = GLFW_KEY_2;
		GetScriptManager().lua["KEY_3"]             = GLFW_KEY_3;
		GetScriptManager().lua["KEY_4"]             = GLFW_KEY_4;
		GetScriptManager().lua["KEY_5"]             = GLFW_KEY_5;
		GetScriptManager().lua["KEY_6"]             = GLFW_KEY_6;
		GetScriptManager().lua["KEY_7"]             = GLFW_KEY_7;
		GetScriptManager().lua["KEY_8"]             = GLFW_KEY_8;
		GetScriptManager().lua["KEY_9"]             = GLFW_KEY_9;
		GetScriptManager().lua["KEY_SEMICOLON"]     = GLFW_KEY_SEMICOLON;
		GetScriptManager().lua["KEY_EQUAL"]         = GLFW_KEY_EQUAL;
		GetScriptManager().lua["KEY_A"]             = GLFW_KEY_A;
		GetScriptManager().lua["KEY_B"]             = GLFW_KEY_B;
		GetScriptManager().lua["KEY_C"]             = GLFW_KEY_C;
		GetScriptManager().lua["KEY_D"]             = GLFW_KEY_D;
		GetScriptManager().lua["KEY_E"]             = GLFW_KEY_E;
		GetScriptManager().lua["KEY_F"]             = GLFW_KEY_F;
		GetScriptManager().lua["KEY_G"]             = GLFW_KEY_G;
		GetScriptManager().lua["KEY_H"]             = GLFW_KEY_H;
		GetScriptManager().lua["KEY_I"]             = GLFW_KEY_I;
		GetScriptManager().lua["KEY_J"]             = GLFW_KEY_J;
		GetScriptManager().lua["KEY_K"]             = GLFW_KEY_K;
		GetScriptManager().lua["KEY_L"]             = GLFW_KEY_L;
		GetScriptManager().lua["KEY_M"]             = GLFW_KEY_M;
		GetScriptManager().lua["KEY_N"]             = GLFW_KEY_N;
		GetScriptManager().lua["KEY_O"]             = GLFW_KEY_O;
		GetScriptManager().lua["KEY_P"]             = GLFW_KEY_P;
		GetScriptManager().lua["KEY_Q"]             = GLFW_KEY_Q;
		GetScriptManager().lua["KEY_R"]             = GLFW_KEY_R;
		GetScriptManager().lua["KEY_S"]             = GLFW_KEY_S;
		GetScriptManager().lua["KEY_T"]             = GLFW_KEY_T;
		GetScriptManager().lua["KEY_U"]             = GLFW_KEY_U;
		GetScriptManager().lua["KEY_V"]             = GLFW_KEY_V;
		GetScriptManager().lua["KEY_W"]             = GLFW_KEY_W;
		GetScriptManager().lua["KEY_X"]             = GLFW_KEY_X;
		GetScriptManager().lua["KEY_Y"]             = GLFW_KEY_Y;
		GetScriptManager().lua["KEY_Z"]             = GLFW_KEY_Z;
		GetScriptManager().lua["KEY_LEFT_BRACKET"]  = GLFW_KEY_LEFT_BRACKET;
		GetScriptManager().lua["KEY_BACKSLASH"]     = GLFW_KEY_BACKSLASH;
		GetScriptManager().lua["KEY_RIGHT_BRACKET"] = GLFW_KEY_RIGHT_BRACKET;
		GetScriptManager().lua["KEY_GRAVE_ACCENT"]  = GLFW_KEY_GRAVE_ACCENT;
		GetScriptManager().lua["KEY_ESCAPE"]        = GLFW_KEY_ESCAPE;
		GetScriptManager().lua["KEY_ENTER"]         = GLFW_KEY_ENTER;
		GetScriptManager().lua["KEY_TAB"]           = GLFW_KEY_TAB;
		GetScriptManager().lua["KEY_BACKSPACE"]     = GLFW_KEY_BACKSPACE;
		GetScriptManager().lua["KEY_INSERT"]        = GLFW_KEY_INSERT;
		GetScriptManager().lua["KEY_DELETE"]        = GLFW_KEY_DELETE;
		GetScriptManager().lua["KEY_RIGHT"]         = GLFW_KEY_RIGHT;
		GetScriptManager().lua["KEY_LEFT"]          = GLFW_KEY_LEFT;
		GetScriptManager().lua["KEY_DOWN"]          = GLFW_KEY_DOWN;
		GetScriptManager().lua["KEY_UP"]            = GLFW_KEY_UP;
		GetScriptManager().lua["KEY_PAGE_UP"]       = GLFW_KEY_PAGE_UP;
		GetScriptManager().lua["KEY_PAGE_DOWN"]     = GLFW_KEY_PAGE_DOWN;
		GetScriptManager().lua["KEY_HOME"]          = GLFW_KEY_HOME;
		GetScriptManager().lua["KEY_END"]           = GLFW_KEY_END;
		GetScriptManager().lua["KEY_CAPS_LOCK"]     = GLFW_KEY_CAPS_LOCK;
		GetScriptManager().lua["KEY_SCROLL_LOCK"]   = GLFW_KEY_SCROLL_LOCK;
		GetScriptManager().lua["KEY_NUM_LOCK"]      = GLFW_KEY_NUM_LOCK;
		GetScriptManager().lua["KEY_PRINT_SCREEN"]  = GLFW_KEY_PRINT_SCREEN;
		GetScriptManager().lua["KEY_PAUSE"]         = GLFW_KEY_PAUSE;
		GetScriptManager().lua["KEY_F1"]            = GLFW_KEY_F1;
		GetScriptManager().lua["KEY_F2"]            = GLFW_KEY_F2;
		GetScriptManager().lua["KEY_F3"]            = GLFW_KEY_F3;
		GetScriptManager().lua["KEY_F4"]            = GLFW_KEY_F4;
		GetScriptManager().lua["KEY_F5"]            = GLFW_KEY_F5;
		GetScriptManager().lua["KEY_F6"]            = GLFW_KEY_F6;
		GetScriptManager().lua["KEY_F7"]            = GLFW_KEY_F7;
		GetScriptManager().lua["KEY_F8"]            = GLFW_KEY_F8;
		GetScriptManager().lua["KEY_F9"]            = GLFW_KEY_F9;
		GetScriptManager().lua["KEY_F10"]           = GLFW_KEY_F10;
		GetScriptManager().lua["KEY_F11"]           = GLFW_KEY_F11;
		GetScriptManager().lua["KEY_F12"]           = GLFW_KEY_F12;
		GetScriptManager().lua["KEY_KP_0"]          = GLFW_KEY_KP_0;
		GetScriptManager().lua["KEY_KP_1"]          = GLFW_KEY_KP_1;
		GetScriptManager().lua["KEY_KP_2"]          = GLFW_KEY_KP_2;
		GetScriptManager().lua["KEY_KP_3"]          = GLFW_KEY_KP_3;
		GetScriptManager().lua["KEY_KP_4"]          = GLFW_KEY_KP_4;
		GetScriptManager().lua["KEY_KP_5"]          = GLFW_KEY_KP_5;
		GetScriptManager().lua["KEY_KP_6"]          = GLFW_KEY_KP_6;
		GetScriptManager().lua["KEY_KP_7"]          = GLFW_KEY_KP_7;
		GetScriptManager().lua["KEY_KP_8"]          = GLFW_KEY_KP_8;
		GetScriptManager().lua["KEY_KP_9"]          = GLFW_KEY_KP_9;
		GetScriptManager().lua["KEY_KP_DECIMAL"]    = GLFW_KEY_KP_DECIMAL;
		GetScriptManager().lua["KEY_KP_DIVIDE"]     = GLFW_KEY_KP_DIVIDE;
		GetScriptManager().lua["KEY_KP_MULTIPLY"]   = GLFW_KEY_KP_MULTIPLY;
		GetScriptManager().lua["KEY_KP_SUBTRACT"]   = GLFW_KEY_KP_SUBTRACT;
		GetScriptManager().lua["KEY_KP_ADD"]        = GLFW_KEY_KP_ADD;
		GetScriptManager().lua["KEY_KP_ENTER"]      = GLFW_KEY_KP_ENTER;
		GetScriptManager().lua["KEY_KP_EQUAL"]      = GLFW_KEY_KP_EQUAL;
		GetScriptManager().lua["KEY_LEFT_SHIFT"]    = GLFW_KEY_LEFT_SHIFT;
		GetScriptManager().lua["KEY_LEFT_CONTROL"]  = GLFW_KEY_LEFT_CONTROL;
		GetScriptManager().lua["KEY_LEFT_ALT"]      = GLFW_KEY_LEFT_ALT;
		GetScriptManager().lua["KEY_LEFT_SUPER"]    = GLFW_KEY_LEFT_SUPER;
		GetScriptManager().lua["KEY_RIGHT_SHIFT"]   = GLFW_KEY_RIGHT_SHIFT;
		GetScriptManager().lua["KEY_RIGHT_CONTROL"] = GLFW_KEY_RIGHT_CONTROL;
		GetScriptManager().lua["KEY_RIGHT_ALT"]     = GLFW_KEY_RIGHT_ALT;
		GetScriptManager().lua["KEY_RIGHT_SUPER"]   = GLFW_KEY_RIGHT_SUPER;
		GetScriptManager().lua["KEY_MENU"]          = GLFW_KEY_MENU;


		GetScriptManager().lua["MOUSE_LEFT"]  = GLFW_MOUSE_BUTTON_LEFT;
		GetScriptManager().lua["MOUSE_RIGHT"] = GLFW_MOUSE_BUTTON_RIGHT;


		GetScriptManager().lua["CURSOR_NORMAL"]   = GLFW_CURSOR_NORMAL;
		GetScriptManager().lua["CURSOR_DISABLED"] = GLFW_CURSOR_DISABLED;
		GetScriptManager().lua["CURSOR_HIDDEN"]   = GLFW_CURSOR_HIDDEN;

		// Global accessor
		GetScriptManager().lua.set_function("getInput", []() -> Input& { return Engine::GetInput(); });
	}


} // namespace Engine