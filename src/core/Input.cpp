#include "Input.h"
#include "EngineData.h"

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>
#include <cstring>

#include "Window.h"
#include "scripting/ScriptManager.h"

namespace Engine {

	namespace {
		constexpr int kMaxGamepadButtons = GLFW_GAMEPAD_BUTTON_LAST + 1;
		constexpr int kMaxGamepadAxes    = GLFW_GAMEPAD_AXIS_LAST + 1;
	} // namespace

	void Input::onShutdown() {}

	void Input::onInit()
	{
		ZoneScopedN("Initialize Input");

		double x, y;
		glfwGetCursorPos(GetWindow().GetNativeWindow(), &x, &y);
		m_lastMousePosition = glm::vec2(static_cast<float>(x), static_cast<float>(y));
		m_mousePosition     = m_lastMousePosition;

		m_gameCursorMode = GLFW_CURSOR_NORMAL;
		SetCursorMode(GLFW_CURSOR_NORMAL);

		std::memset(&m_gamepadState, 0, sizeof(m_gamepadState));
		std::memset(&m_prevGamepadState, 0, sizeof(m_prevGamepadState));

		RegisterDefaultAxes();
		UpdateGamepadState();
		UpdateAxes(0.0f);
	}

	void Input::RegisterDefaultAxes()
	{
		m_axes.clear();

		// Horizontal: A/D, arrows, left stick X, d-pad left/right
		{
			InputAxis a;
			a.name = "Horizontal";
			a.positiveKeys = {GLFW_KEY_D, GLFW_KEY_RIGHT};
			a.negativeKeys = {GLFW_KEY_A, GLFW_KEY_LEFT};
			a.gamepadAxis           = GLFW_GAMEPAD_AXIS_LEFT_X;
			a.gamepadAxisScale      = 1.0f;
			a.gamepadPositiveButton = GLFW_GAMEPAD_BUTTON_DPAD_RIGHT;
			a.gamepadNegativeButton = GLFW_GAMEPAD_BUTTON_DPAD_LEFT;
			a.deadzone              = m_gamepadDeadzone;
			m_axes.push_back(a);
		}

		// Vertical: W/S, arrows, left stick Y (inverted so up = +1), d-pad
		{
			InputAxis a;
			a.name = "Vertical";
			a.positiveKeys = {GLFW_KEY_W, GLFW_KEY_UP};
			a.negativeKeys = {GLFW_KEY_S, GLFW_KEY_DOWN};
			a.gamepadAxis           = GLFW_GAMEPAD_AXIS_LEFT_Y;
			a.gamepadAxisScale      = -1.0f; // GLFW/Xbox: stick up is typically negative Y
			a.gamepadPositiveButton = GLFW_GAMEPAD_BUTTON_DPAD_UP;
			a.gamepadNegativeButton = GLFW_GAMEPAD_BUTTON_DPAD_DOWN;
			a.deadzone              = m_gamepadDeadzone;
			m_axes.push_back(a);
		}

		// Mouse look deltas (raw = pixels this frame; smoothed still useful for light filtering)
		{
			InputAxis a;
			a.name        = "Mouse X";
			a.sensitivity = 1.0f;
			a.gravity     = 0.0f;
			a.snap        = false;
			a.deadzone    = 0.0f;
			m_axes.push_back(a);
		}
		{
			InputAxis a;
			a.name        = "Mouse Y";
			a.sensitivity = 1.0f;
			a.gravity     = 0.0f;
			a.snap        = false;
			a.deadzone    = 0.0f;
			m_axes.push_back(a);
		}

		// Right stick look (analog)
		{
			InputAxis a;
			a.name             = "Look Horizontal";
			a.gamepadAxis      = GLFW_GAMEPAD_AXIS_RIGHT_X;
			a.gamepadAxisScale = 1.0f;
			a.deadzone         = m_gamepadDeadzone;
			m_axes.push_back(a);
		}
		{
			InputAxis a;
			a.name             = "Look Vertical";
			a.gamepadAxis      = GLFW_GAMEPAD_AXIS_RIGHT_Y;
			a.gamepadAxisScale = -1.0f; // up = positive
			a.deadzone         = m_gamepadDeadzone;
			m_axes.push_back(a);
		}

		for (const auto& axis : m_axes) {
			m_axisRaw[axis.name]      = 0.0f;
			m_axisSmoothed[axis.name] = 0.0f;
		}
	}

	void Input::UpdateGamepadState()
	{
		m_prevGamepadState = m_gamepadState;

		m_gamepadConnected = false;
		std::memset(&m_gamepadState, 0, sizeof(m_gamepadState));

		// Prefer the first present gamepad (scan GLFW_JOYSTICK_1 .. LAST)
		m_activeGamepadJid = GLFW_JOYSTICK_1;
		for (int jid = GLFW_JOYSTICK_1; jid <= GLFW_JOYSTICK_LAST; ++jid) {
			if (glfwJoystickPresent(jid) && glfwJoystickIsGamepad(jid)) {
				m_activeGamepadJid = jid;
				if (glfwGetGamepadState(jid, &m_gamepadState) == GLFW_TRUE) {
					m_gamepadConnected = true;
				}
				break;
			}
		}
	}

	float Input::ApplyDeadzone(float value, float deadzone) const
	{
		const float dz = std::max(0.0f, deadzone);
		if (dz <= 0.0f) {
			return std::clamp(value, -1.0f, 1.0f);
		}
		const float absVal = std::fabs(value);
		if (absVal < dz) {
			return 0.0f;
		}
		// Rescale so output goes 0..1 past the deadzone edge
		const float sign   = value < 0.0f ? -1.0f : 1.0f;
		const float scaled = (absVal - dz) / (1.0f - dz);
		return sign * std::clamp(scaled, 0.0f, 1.0f);
	}

	float Input::ComputeAxisRaw(const InputAxis& axis) const
	{
		// Special cases: mouse deltas (pixels this frame)
		if (axis.name == "Mouse X") {
			return GetMouseDelta().x;
		}
		if (axis.name == "Mouse Y") {
			return GetMouseDelta().y;
		}

		float value = 0.0f;

		for (int key : axis.positiveKeys) {
			if (IsKeyPressed(key)) {
				value += 1.0f;
				break;
			}
		}
		for (int key : axis.negativeKeys) {
			if (IsKeyPressed(key)) {
				value -= 1.0f;
				break;
			}
		}

		if (m_gamepadConnected) {
			float pad = 0.0f;
			if (axis.gamepadAxis >= 0 && axis.gamepadAxis < kMaxGamepadAxes) {
				pad = ApplyDeadzone(m_gamepadState.axes[axis.gamepadAxis] * axis.gamepadAxisScale, axis.deadzone);
			}
			if (axis.gamepadPositiveButton >= 0 &&
			    axis.gamepadPositiveButton < kMaxGamepadButtons &&
			    m_gamepadState.buttons[axis.gamepadPositiveButton] == GLFW_PRESS) {
				pad += 1.0f;
			}
			if (axis.gamepadNegativeButton >= 0 &&
			    axis.gamepadNegativeButton < kMaxGamepadButtons &&
			    m_gamepadState.buttons[axis.gamepadNegativeButton] == GLFW_PRESS) {
				pad -= 1.0f;
			}
			pad = std::clamp(pad, -1.0f, 1.0f);
			// Keyboard digital + pad: keep the contribution with larger magnitude
			if (std::fabs(pad) > std::fabs(value)) {
				value = pad;
			}
		}

		return std::clamp(value, -1.0f, 1.0f);
	}

	void Input::UpdateAxes(float dt)
	{
		for (const InputAxis& axis : m_axes) {
			const float target = ComputeAxisRaw(axis);
			m_axisRaw[axis.name] = target;

			// Mouse axes: no smoothing toward zero — report current delta only
			if (axis.name == "Mouse X" || axis.name == "Mouse Y") {
				m_axisSmoothed[axis.name] = target;
				continue;
			}

			float current = m_axisSmoothed[axis.name];

			if (axis.snap && target != 0.0f && ((current > 0.0f && target < 0.0f) || (current < 0.0f && target > 0.0f))) {
				current = 0.0f;
			}

			if (std::fabs(target) > 0.0f) {
				// Move toward target
				const float step = axis.sensitivity * dt;
				if (current < target) {
					current = std::min(current + step, target);
				}
				else if (current > target) {
					current = std::max(current - step, target);
				}
			}
			else {
				// Fall back to zero
				const float step = axis.gravity * dt;
				if (current > 0.0f) {
					current = std::max(current - step, 0.0f);
				}
				else if (current < 0.0f) {
					current = std::min(current + step, 0.0f);
				}
			}

			m_axisSmoothed[axis.name] = std::clamp(current, -1.0f, 1.0f);
		}
	}

	const InputAxis* Input::FindAxis(const std::string& name) const
	{
		for (const auto& axis : m_axes) {
			if (axis.name == name) {
				return &axis;
			}
		}
		return nullptr;
	}

	float Input::GetAxis(const std::string& name) const
	{
		auto it = m_axisSmoothed.find(name);
		if (it == m_axisSmoothed.end()) {
			return 0.0f;
		}
		return it->second;
	}

	float Input::GetAxisRaw(const std::string& name) const
	{
		auto it = m_axisRaw.find(name);
		if (it == m_axisRaw.end()) {
			return 0.0f;
		}
		return it->second;
	}

	bool Input::IsGamepadConnected(int jid) const
	{
		if (jid < 0) {
			return m_gamepadConnected;
		}
		return glfwJoystickPresent(jid) && glfwJoystickIsGamepad(jid);
	}

	float Input::GetGamepadAxis(int axis, int jid) const
	{
		if (axis < 0 || axis >= kMaxGamepadAxes) {
			return 0.0f;
		}

		GLFWgamepadstate state{};
		const int        useJid = (jid < 0) ? m_activeGamepadJid : jid;
		if (!glfwJoystickPresent(useJid) || !glfwJoystickIsGamepad(useJid)) {
			return 0.0f;
		}
		if (glfwGetGamepadState(useJid, &state) != GLFW_TRUE) {
			return 0.0f;
		}

		float v = state.axes[axis];
		// Triggers are 0..1 resting at -1 or 0 depending on mapping; leave raw for triggers
		if (axis == GLFW_GAMEPAD_AXIS_LEFT_TRIGGER || axis == GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER) {
			return std::clamp(v, -1.0f, 1.0f);
		}
		return ApplyDeadzone(v, m_gamepadDeadzone);
	}

	bool Input::IsGamepadButtonPressed(int button, int jid) const
	{
		if (button < 0 || button >= kMaxGamepadButtons) {
			return false;
		}

		// Fast path: primary pad state updated each frame
		if ((jid < 0 || jid == m_activeGamepadJid) && m_gamepadConnected) {
			return m_gamepadState.buttons[button] == GLFW_PRESS;
		}

		GLFWgamepadstate state{};
		if (!glfwJoystickPresent(jid) || !glfwJoystickIsGamepad(jid)) {
			return false;
		}
		if (glfwGetGamepadState(jid, &state) != GLFW_TRUE) {
			return false;
		}
		return state.buttons[button] == GLFW_PRESS;
	}

	bool Input::IsGamepadButtonPressedThisFrame(int button, int jid) const
	{
		if (button < 0 || button >= kMaxGamepadButtons) {
			return false;
		}
		if ((jid < 0 || jid == m_activeGamepadJid) && m_gamepadConnected) {
			const bool now  = m_gamepadState.buttons[button] == GLFW_PRESS;
			const bool prev = m_prevGamepadState.buttons[button] == GLFW_PRESS;
			return now && !prev;
		}
		// No edge tracking for non-active pads
		return IsGamepadButtonPressed(button, jid);
	}

	bool Input::IsGamepadButtonReleasedThisFrame(int button, int jid) const
	{
		if (button < 0 || button >= kMaxGamepadButtons) {
			return false;
		}
		if ((jid < 0 || jid == m_activeGamepadJid) && m_gamepadConnected) {
			const bool now  = m_gamepadState.buttons[button] == GLFW_PRESS;
			const bool prev = m_prevGamepadState.buttons[button] == GLFW_PRESS;
			return !now && prev;
		}
		return false;
	}

	std::string Input::GetGamepadName(int jid) const
	{
		const int useJid = (jid < 0) ? m_activeGamepadJid : jid;
		if (!glfwJoystickPresent(useJid) || !glfwJoystickIsGamepad(useJid)) {
			return {};
		}
		const char* name = glfwGetGamepadName(useJid);
		return name ? std::string(name) : std::string{};
	}

	void Input::SetGamepadDeadzone(float deadzone)
	{
		m_gamepadDeadzone = std::clamp(deadzone, 0.0f, 0.95f);
		for (auto& axis : m_axes) {
			if (axis.gamepadAxis >= 0) {
				axis.deadzone = m_gamepadDeadzone;
			}
		}
	}

	void Input::onUpdate(float dt)
	{
		ZoneScoped;
		if (GetState() == PLAYING) {
			// Hold Escape: free cursor for ImGui (overrides script setCursorMode(DISABLED)).
			if (IsKeyPressed(GLFW_KEY_ESCAPE)) {
				m_gameCursorMode = GLFW_CURSOR_NORMAL;
			}
			GLFWwindow* win = GetWindow().GetNativeWindow();
			glfwSetInputMode(win, GLFW_CURSOR, m_gameCursorMode);
			// Raw deltas avoid integer cursor quantization so look speed matches across FPS.
			if (glfwRawMouseMotionSupported()) {
				const int raw = (m_gameCursorMode == GLFW_CURSOR_DISABLED) ? GLFW_TRUE : GLFW_FALSE;
				glfwSetInputMode(win, GLFW_RAW_MOUSE_MOTION, raw);
			}
		}

		m_lastMousePosition = m_mousePosition;
		double x, y;
		glfwGetCursorPos(GetWindow().GetNativeWindow(), &x, &y);
		m_mousePosition = glm::vec2(static_cast<float>(x), static_cast<float>(y));

		m_prevKeyStates = m_keyStates;
		m_keyStates.clear();
		for (const auto& pair : m_prevKeyStates) {
			int key          = pair.first;
			m_keyStates[key] = IsKeyPressed(key);
		}

		m_prevMouseButtonStates = m_mouseButtonStates;
		m_mouseButtonStates[GLFW_MOUSE_BUTTON_LEFT]   = glfwGetMouseButton(GetWindow().GetNativeWindow(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
		m_mouseButtonStates[GLFW_MOUSE_BUTTON_RIGHT]  = glfwGetMouseButton(GetWindow().GetNativeWindow(), GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
		m_mouseButtonStates[GLFW_MOUSE_BUTTON_MIDDLE] = glfwGetMouseButton(GetWindow().GetNativeWindow(), GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;

		UpdateGamepadState();
		UpdateAxes(dt);
	}

	bool Input::IsMouseClicked(int btn)
	{
		bool current  = m_mouseButtonStates[btn];
		bool previous = false;
		auto it       = m_prevMouseButtonStates.find(btn);
		if (it != m_prevMouseButtonStates.end()) previous = it->second;

		return current && !previous;
	}

	bool Input::IsMousePressed(int btn) const
	{
		return glfwGetMouseButton(GetWindow().GetNativeWindow(), btn) == GLFW_PRESS;
	}

	bool Input::IsKeyPressed(int key) const
	{
		return glfwGetKey(GetWindow().GetNativeWindow(), key) == GLFW_PRESS;
	}

	bool Input::IsKeyReleased(int key) const
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
#ifndef GAME_BUILD
		if (ImGui::IsKeyDown(ImGuiKey_Escape)) return {0, 0};
#endif
		return glm::vec2(m_mousePosition.x, m_lastMousePosition.y) - glm::vec2(m_lastMousePosition.x, m_mousePosition.y);
	}

	float Input::GetMouseScrollDelta() const
	{
		return m_scrollDelta;
	}

	void Input::ResetScroll()
	{
		m_scrollDelta = 0;
	}

	[[maybe_unused]] void Input::SetMousePosition(const glm::vec2& pos)
	{
		glfwSetCursorPos(GetWindow().GetNativeWindow(), pos.x, pos.y);
	}

	void Input::SetCursorModeGame(int mode)
	{
		// While Escape is held in play mode, ignore recapture so ImGui can use the mouse.
		if (GetState() == PLAYING && mode == GLFW_CURSOR_DISABLED && IsKeyPressed(GLFW_KEY_ESCAPE)) {
			m_gameCursorMode = GLFW_CURSOR_NORMAL;
			return;
		}
		m_gameCursorMode = mode;
	}

	int Input::GetCursorModeGame() const
	{
		return m_gameCursorMode;
	}

	void Input::SetCursorMode(int mode)
	{
		GLFWwindow* win = GetWindow().GetNativeWindow();
		glfwSetInputMode(win, GLFW_CURSOR, mode);
		if (glfwRawMouseMotionSupported()) {
			const int raw = (mode == GLFW_CURSOR_DISABLED) ? GLFW_TRUE : GLFW_FALSE;
			glfwSetInputMode(win, GLFW_RAW_MOUSE_MOTION, raw);
		}
	}

	int Input::GetCursorMode()
	{
		return glfwGetInputMode(GetWindow().GetNativeWindow(), GLFW_CURSOR);
	}

	void Input::ScrollCallback(GLFWwindow* /*wnd*/, double /*xoff*/, double yoff)
	{
		GetInput().m_scrollDelta += static_cast<float>(yoff);
	}

	bool Input::IsKeyPressedThisFrame(int key)
	{
		if (m_keyStates.find(key) == m_keyStates.end()) {
			m_keyStates[key] = IsKeyPressed(key);
		}

		bool currentState = m_keyStates[key];

		bool previousState = false;
		auto it            = m_prevKeyStates.find(key);
		if (it != m_prevKeyStates.end()) {
			previousState = it->second;
		}

		return currentState && !previousState;
	}

	void Input::setLuaBindings()
	{
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
		                                           &Input::GetCursorModeGame,
		                                           "isMouseClicked",
		                                           &Input::IsMouseClicked,

		                                           // Virtual axes (Unity-style)
		                                           "getAxis",
		                                           &Input::GetAxis,
		                                           "getAxisRaw",
		                                           &Input::GetAxisRaw,

		                                           // Gamepad
		                                           "isGamepadConnected",
		                                           sol::overload(
		                                               [](Input& self) { return self.IsGamepadConnected(); },
		                                               [](Input& self, int jid) { return self.IsGamepadConnected(jid); }),
		                                           "getGamepadAxis",
		                                           sol::overload(
		                                               [](Input& self, int axis) { return self.GetGamepadAxis(axis); },
		                                               [](Input& self, int axis, int jid) { return self.GetGamepadAxis(axis, jid); }),
		                                           "isGamepadButtonPressed",
		                                           sol::overload(
		                                               [](Input& self, int button) { return self.IsGamepadButtonPressed(button); },
		                                               [](Input& self, int button, int jid) { return self.IsGamepadButtonPressed(button, jid); }),
		                                           "isGamepadButtonPressedThisFrame",
		                                           sol::overload(
		                                               [](Input& self, int button) { return self.IsGamepadButtonPressedThisFrame(button); },
		                                               [](Input& self, int button, int jid) { return self.IsGamepadButtonPressedThisFrame(button, jid); }),
		                                           "isGamepadButtonReleasedThisFrame",
		                                           sol::overload(
		                                               [](Input& self, int button) { return self.IsGamepadButtonReleasedThisFrame(button); },
		                                               [](Input& self, int button, int jid) { return self.IsGamepadButtonReleasedThisFrame(button, jid); }),
		                                           "getGamepadName",
		                                           sol::overload(
		                                               [](Input& self) { return self.GetGamepadName(); },
		                                               [](Input& self, int jid) { return self.GetGamepadName(jid); }),
		                                           "setGamepadDeadzone",
		                                           &Input::SetGamepadDeadzone,
		                                           "getGamepadDeadzone",
		                                           &Input::GetGamepadDeadzone);

		auto& lua = GetScriptManager().lua;

		lua["KEY_SPACE"]         = GLFW_KEY_SPACE;
		lua["KEY_APOSTROPHE"]    = GLFW_KEY_APOSTROPHE;
		lua["KEY_COMMA"]         = GLFW_KEY_COMMA;
		lua["KEY_MINUS"]         = GLFW_KEY_MINUS;
		lua["KEY_PERIOD"]        = GLFW_KEY_PERIOD;
		lua["KEY_SLASH"]         = GLFW_KEY_SLASH;
		lua["KEY_0"]             = GLFW_KEY_0;
		lua["KEY_1"]             = GLFW_KEY_1;
		lua["KEY_2"]             = GLFW_KEY_2;
		lua["KEY_3"]             = GLFW_KEY_3;
		lua["KEY_4"]             = GLFW_KEY_4;
		lua["KEY_5"]             = GLFW_KEY_5;
		lua["KEY_6"]             = GLFW_KEY_6;
		lua["KEY_7"]             = GLFW_KEY_7;
		lua["KEY_8"]             = GLFW_KEY_8;
		lua["KEY_9"]             = GLFW_KEY_9;
		lua["KEY_SEMICOLON"]     = GLFW_KEY_SEMICOLON;
		lua["KEY_EQUAL"]         = GLFW_KEY_EQUAL;
		lua["KEY_A"]             = GLFW_KEY_A;
		lua["KEY_B"]             = GLFW_KEY_B;
		lua["KEY_C"]             = GLFW_KEY_C;
		lua["KEY_D"]             = GLFW_KEY_D;
		lua["KEY_E"]             = GLFW_KEY_E;
		lua["KEY_F"]             = GLFW_KEY_F;
		lua["KEY_G"]             = GLFW_KEY_G;
		lua["KEY_H"]             = GLFW_KEY_H;
		lua["KEY_I"]             = GLFW_KEY_I;
		lua["KEY_J"]             = GLFW_KEY_J;
		lua["KEY_K"]             = GLFW_KEY_K;
		lua["KEY_L"]             = GLFW_KEY_L;
		lua["KEY_M"]             = GLFW_KEY_M;
		lua["KEY_N"]             = GLFW_KEY_N;
		lua["KEY_O"]             = GLFW_KEY_O;
		lua["KEY_P"]             = GLFW_KEY_P;
		lua["KEY_Q"]             = GLFW_KEY_Q;
		lua["KEY_R"]             = GLFW_KEY_R;
		lua["KEY_S"]             = GLFW_KEY_S;
		lua["KEY_T"]             = GLFW_KEY_T;
		lua["KEY_U"]             = GLFW_KEY_U;
		lua["KEY_V"]             = GLFW_KEY_V;
		lua["KEY_W"]             = GLFW_KEY_W;
		lua["KEY_X"]             = GLFW_KEY_X;
		lua["KEY_Y"]             = GLFW_KEY_Y;
		lua["KEY_Z"]             = GLFW_KEY_Z;
		lua["KEY_LEFT_BRACKET"]  = GLFW_KEY_LEFT_BRACKET;
		lua["KEY_BACKSLASH"]     = GLFW_KEY_BACKSLASH;
		lua["KEY_RIGHT_BRACKET"] = GLFW_KEY_RIGHT_BRACKET;
		lua["KEY_GRAVE_ACCENT"]  = GLFW_KEY_GRAVE_ACCENT;
		lua["KEY_ESCAPE"]        = GLFW_KEY_ESCAPE;
		lua["KEY_ENTER"]         = GLFW_KEY_ENTER;
		lua["KEY_TAB"]           = GLFW_KEY_TAB;
		lua["KEY_BACKSPACE"]     = GLFW_KEY_BACKSPACE;
		lua["KEY_INSERT"]        = GLFW_KEY_INSERT;
		lua["KEY_DELETE"]        = GLFW_KEY_DELETE;
		lua["KEY_RIGHT"]         = GLFW_KEY_RIGHT;
		lua["KEY_LEFT"]          = GLFW_KEY_LEFT;
		lua["KEY_DOWN"]          = GLFW_KEY_DOWN;
		lua["KEY_UP"]            = GLFW_KEY_UP;
		lua["KEY_PAGE_UP"]       = GLFW_KEY_PAGE_UP;
		lua["KEY_PAGE_DOWN"]     = GLFW_KEY_PAGE_DOWN;
		lua["KEY_HOME"]          = GLFW_KEY_HOME;
		lua["KEY_END"]           = GLFW_KEY_END;
		lua["KEY_CAPS_LOCK"]     = GLFW_KEY_CAPS_LOCK;
		lua["KEY_SCROLL_LOCK"]   = GLFW_KEY_SCROLL_LOCK;
		lua["KEY_NUM_LOCK"]      = GLFW_KEY_NUM_LOCK;
		lua["KEY_PRINT_SCREEN"]  = GLFW_KEY_PRINT_SCREEN;
		lua["KEY_PAUSE"]         = GLFW_KEY_PAUSE;
		lua["KEY_F1"]            = GLFW_KEY_F1;
		lua["KEY_F2"]            = GLFW_KEY_F2;
		lua["KEY_F3"]            = GLFW_KEY_F3;
		lua["KEY_F4"]            = GLFW_KEY_F4;
		lua["KEY_F5"]            = GLFW_KEY_F5;
		lua["KEY_F6"]            = GLFW_KEY_F6;
		lua["KEY_F7"]            = GLFW_KEY_F7;
		lua["KEY_F8"]            = GLFW_KEY_F8;
		lua["KEY_F9"]            = GLFW_KEY_F9;
		lua["KEY_F10"]           = GLFW_KEY_F10;
		lua["KEY_F11"]           = GLFW_KEY_F11;
		lua["KEY_F12"]           = GLFW_KEY_F12;
		lua["KEY_KP_0"]          = GLFW_KEY_KP_0;
		lua["KEY_KP_1"]          = GLFW_KEY_KP_1;
		lua["KEY_KP_2"]          = GLFW_KEY_KP_2;
		lua["KEY_KP_3"]          = GLFW_KEY_KP_3;
		lua["KEY_KP_4"]          = GLFW_KEY_KP_4;
		lua["KEY_KP_5"]          = GLFW_KEY_KP_5;
		lua["KEY_KP_6"]          = GLFW_KEY_KP_6;
		lua["KEY_KP_7"]          = GLFW_KEY_KP_7;
		lua["KEY_KP_8"]          = GLFW_KEY_KP_8;
		lua["KEY_KP_9"]          = GLFW_KEY_KP_9;
		lua["KEY_KP_DECIMAL"]    = GLFW_KEY_KP_DECIMAL;
		lua["KEY_KP_DIVIDE"]     = GLFW_KEY_KP_DIVIDE;
		lua["KEY_KP_MULTIPLY"]   = GLFW_KEY_KP_MULTIPLY;
		lua["KEY_KP_SUBTRACT"]   = GLFW_KEY_KP_SUBTRACT;
		lua["KEY_KP_ADD"]        = GLFW_KEY_KP_ADD;
		lua["KEY_KP_ENTER"]      = GLFW_KEY_KP_ENTER;
		lua["KEY_KP_EQUAL"]      = GLFW_KEY_KP_EQUAL;
		lua["KEY_LEFT_SHIFT"]    = GLFW_KEY_LEFT_SHIFT;
		lua["KEY_LEFT_CONTROL"]  = GLFW_KEY_LEFT_CONTROL;
		lua["KEY_LEFT_ALT"]      = GLFW_KEY_LEFT_ALT;
		lua["KEY_LEFT_SUPER"]    = GLFW_KEY_LEFT_SUPER;
		lua["KEY_RIGHT_SHIFT"]   = GLFW_KEY_RIGHT_SHIFT;
		lua["KEY_RIGHT_CONTROL"] = GLFW_KEY_RIGHT_CONTROL;
		lua["KEY_RIGHT_ALT"]     = GLFW_KEY_RIGHT_ALT;
		lua["KEY_RIGHT_SUPER"]   = GLFW_KEY_RIGHT_SUPER;
		lua["KEY_MENU"]          = GLFW_KEY_MENU;

		lua["MOUSE_LEFT"]  = GLFW_MOUSE_BUTTON_LEFT;
		lua["MOUSE_RIGHT"] = GLFW_MOUSE_BUTTON_RIGHT;

		lua["CURSOR_NORMAL"]   = GLFW_CURSOR_NORMAL;
		lua["CURSOR_DISABLED"] = GLFW_CURSOR_DISABLED;
		lua["CURSOR_HIDDEN"]   = GLFW_CURSOR_HIDDEN;

		// Gamepad buttons (Xbox layout via GLFW)
		lua["GAMEPAD_A"]            = GLFW_GAMEPAD_BUTTON_A;
		lua["GAMEPAD_B"]            = GLFW_GAMEPAD_BUTTON_B;
		lua["GAMEPAD_X"]            = GLFW_GAMEPAD_BUTTON_X;
		lua["GAMEPAD_Y"]            = GLFW_GAMEPAD_BUTTON_Y;
		lua["GAMEPAD_LEFT_BUMPER"]  = GLFW_GAMEPAD_BUTTON_LEFT_BUMPER;
		lua["GAMEPAD_RIGHT_BUMPER"] = GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER;
		lua["GAMEPAD_BACK"]         = GLFW_GAMEPAD_BUTTON_BACK;
		lua["GAMEPAD_START"]        = GLFW_GAMEPAD_BUTTON_START;
		lua["GAMEPAD_GUIDE"]        = GLFW_GAMEPAD_BUTTON_GUIDE;
		lua["GAMEPAD_LEFT_THUMB"]   = GLFW_GAMEPAD_BUTTON_LEFT_THUMB;
		lua["GAMEPAD_RIGHT_THUMB"]  = GLFW_GAMEPAD_BUTTON_RIGHT_THUMB;
		lua["GAMEPAD_DPAD_UP"]      = GLFW_GAMEPAD_BUTTON_DPAD_UP;
		lua["GAMEPAD_DPAD_RIGHT"]   = GLFW_GAMEPAD_BUTTON_DPAD_RIGHT;
		lua["GAMEPAD_DPAD_DOWN"]    = GLFW_GAMEPAD_BUTTON_DPAD_DOWN;
		lua["GAMEPAD_DPAD_LEFT"]    = GLFW_GAMEPAD_BUTTON_DPAD_LEFT;
		// PlayStation aliases
		lua["GAMEPAD_CROSS"]    = GLFW_GAMEPAD_BUTTON_CROSS;
		lua["GAMEPAD_CIRCLE"]   = GLFW_GAMEPAD_BUTTON_CIRCLE;
		lua["GAMEPAD_SQUARE"]   = GLFW_GAMEPAD_BUTTON_SQUARE;
		lua["GAMEPAD_TRIANGLE"] = GLFW_GAMEPAD_BUTTON_TRIANGLE;

		// Gamepad axes
		lua["GAMEPAD_AXIS_LEFT_X"]        = GLFW_GAMEPAD_AXIS_LEFT_X;
		lua["GAMEPAD_AXIS_LEFT_Y"]        = GLFW_GAMEPAD_AXIS_LEFT_Y;
		lua["GAMEPAD_AXIS_RIGHT_X"]       = GLFW_GAMEPAD_AXIS_RIGHT_X;
		lua["GAMEPAD_AXIS_RIGHT_Y"]       = GLFW_GAMEPAD_AXIS_RIGHT_Y;
		lua["GAMEPAD_AXIS_LEFT_TRIGGER"]  = GLFW_GAMEPAD_AXIS_LEFT_TRIGGER;
		lua["GAMEPAD_AXIS_RIGHT_TRIGGER"] = GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER;

		lua.set_function("getInput", []() -> Input& { return Engine::GetInput(); });
	}

} // namespace Engine
