#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <unordered_map>

namespace Engine {
	class Input {
	  public:
		static void Initialize(GLFWwindow* window);
		static void Update();

		// Keyboard input
		static bool IsKeyPressed(int key);
		static bool IsKeyReleased(int key);
		static bool IsKeyPressedThisFrame(int key);

		// Mouse input
		static bool                       IsMousePressed(int btn);
		[[maybe_unused]] static glm::vec2 GetMousePosition();
		static glm::vec2                  GetMouseDelta();
		static float                      GetMouseScrollDelta();
		[[maybe_unused]] static void      SetMousePosition(const glm::vec2& position);
		static void                       SetCursorMode(int mode);
		static int                        GetCursorMode();

		// Callbacks
		[[maybe_unused]] static void MouseCallback(GLFWwindow* window, double xpos, double ypos);
		static void                  ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

	  private:
		static GLFWwindow*                   s_window;
		static glm::vec2                     s_mousePosition;
		static glm::vec2                     s_lastMousePosition;
		static float                         s_scrollDelta;
		static std::unordered_map<int, bool> s_keyStates;     // Current frame
		static std::unordered_map<int, bool> s_prevKeyStates; // Previous frame
	};
} // namespace Engine