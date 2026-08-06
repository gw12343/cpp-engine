#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <string>
#include <unordered_map>
#include <vector>
#include "core/module/Module.h"

namespace Engine {
	/// Named virtual axis (Unity-style). Combined from keys, gamepad axes, and optional buttons.
	struct InputAxis {
		std::string name;

		// Keyboard: positive contributes +1, negative contributes -1 (sum clamped to [-1, 1])
		std::vector<int> positiveKeys;
		std::vector<int> negativeKeys;

		// Gamepad stick / trigger axis (GLFW_GAMEPAD_AXIS_*), or -1 if unused
		int   gamepadAxis      = -1;
		float gamepadAxisScale = 1.0f; // use -1 to invert (e.g. stick Y)

		// Digital gamepad buttons contributing ±1, or -1 if unused
		int gamepadPositiveButton = -1;
		int gamepadNegativeButton = -1;

		float deadzone    = 0.2f;
		float sensitivity = 3.0f; // toward target (GetAxis smoothing)
		float gravity     = 3.0f; // toward zero when released
		bool  snap        = true;  // snap when reversing direction
	};

	class Input : public Module {
	  public:
		Input() = default;

		void        onInit() override;
		void        onUpdate(float dt) override;
		void        onGameStart() override {}
		void        onShutdown() override;
		std::string name() const override { return "InputModule"; };
		void        setLuaBindings() override;

		// Keyboard input
		bool IsKeyPressed(int key) const;
		bool IsKeyReleased(int key) const;
		bool IsKeyPressedThisFrame(int key);

		// Mouse input
		bool                       IsMousePressed(int btn) const;
		[[maybe_unused]] glm::vec2 GetMousePosition();
		bool                       IsMousePositionInViewport() const;
		glm::vec2                  GetMousePositionInViewport() const;
		glm::vec2                  GetMousePositionInViewportScaledFlipped() const;
		glm::vec2                  GetMouseDelta() const;
		float                      GetMouseScrollDelta() const;
		[[maybe_unused]] static void SetMousePosition(const glm::vec2& position);

		static void SetCursorMode(int mode);
		static int  GetCursorMode();
		void        SetCursorModeGame(int mode);
		int         GetCursorModeGame() const;
		bool        IsMouseClicked(int btn);
		void        ResetScroll();

		// --- Virtual axes (Unity-like) ---
		/// Smoothed axis in [-1, 1]. Use for analog-feel movement.
		float GetAxis(const std::string& name) const;
		/// Instant keyboard/stick value in [-1, 1] (no smoothing). Prefer for responsive movement.
		float GetAxisRaw(const std::string& name) const;

		// --- Gamepad ---
		/// True if joystick jid is present and has a standard gamepad mapping (default: first pad).
		bool IsGamepadConnected(int jid = GLFW_JOYSTICK_1) const;
		/// Left stick etc. after deadzone, range typically [-1, 1] (triggers [0, 1]).
		float GetGamepadAxis(int axis, int jid = GLFW_JOYSTICK_1) const;
		bool  IsGamepadButtonPressed(int button, int jid = GLFW_JOYSTICK_1) const;
		bool  IsGamepadButtonPressedThisFrame(int button, int jid = GLFW_JOYSTICK_1) const;
		bool  IsGamepadButtonReleasedThisFrame(int button, int jid = GLFW_JOYSTICK_1) const;
		/// Human-readable name, or empty if disconnected.
		std::string GetGamepadName(int jid = GLFW_JOYSTICK_1) const;

		// Deadzone applied to stick axes when reading via GetGamepadAxis / virtual axes
		void  SetGamepadDeadzone(float deadzone);
		float GetGamepadDeadzone() const { return m_gamepadDeadzone; }

		std::unordered_map<int, bool> m_mouseButtonStates;
		std::unordered_map<int, bool> m_prevMouseButtonStates;

		static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

	  private:
		void RegisterDefaultAxes();
		void UpdateGamepadState();
		void UpdateAxes(float dt);
		float ComputeAxisRaw(const InputAxis& axis) const;
		float ApplyDeadzone(float value, float deadzone) const;
		const InputAxis* FindAxis(const std::string& name) const;

		glm::vec2 m_mousePosition{};
		glm::vec2 m_lastMousePosition{};
		float     m_scrollDelta = 0.0f;

		std::unordered_map<int, bool> m_keyStates;
		std::unordered_map<int, bool> m_prevKeyStates;

		int m_gameCursorMode = GLFW_CURSOR_NORMAL;

		// Gamepad (primary = GLFW_JOYSTICK_1)
		bool             m_gamepadConnected = false;
		GLFWgamepadstate m_gamepadState{};
		GLFWgamepadstate m_prevGamepadState{};
		float            m_gamepadDeadzone = 0.2f;
		int              m_activeGamepadJid = GLFW_JOYSTICK_1;

		std::vector<InputAxis>                m_axes;
		std::unordered_map<std::string, float> m_axisRaw;
		std::unordered_map<std::string, float> m_axisSmoothed;
	};
} // namespace Engine
