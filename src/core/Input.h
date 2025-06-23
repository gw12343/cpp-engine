#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include "core/module/Module.h"

namespace Engine {
	class Input : public Module {
	  public:
		Input() = default;


		void        onInit() override;
		void        onUpdate(float dt) override;
		void        onShutdown() override;
		std::string name() const override { return "InputModule"; };
		void        setLuaBindings() override;

		// Keyboard input
		bool IsKeyPressed(int key);
		bool IsKeyReleased(int key);
		bool IsKeyPressedThisFrame(int key);

		// Mouse input
		bool                       IsMousePressed(int btn);
		[[maybe_unused]] glm::vec2 GetMousePosition();
		glm::vec2                  GetMouseDelta() const;
		// float                      GetMouseScrollDelta();
		[[maybe_unused]] void SetMousePosition(const glm::vec2& position);
		void                  SetCursorMode(int mode);
		int                   GetCursorMode();


		//		// Callbacks
		//		[[maybe_unused]] void MouseCallback(GLFWwindow* window, double xpos, double ypos);
		//		void                  ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);


	  private:
		glm::vec2 m_mousePosition;
		glm::vec2 m_lastMousePosition;
		// float                         m_scrollDelta;
		std::unordered_map<int, bool> m_keyStates;     // Current frame
		std::unordered_map<int, bool> m_prevKeyStates; // Previous frame
	};
} // namespace Engine