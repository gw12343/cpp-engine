#pragma once
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <memory>
#include <string>

namespace Engine {

	class Window {
	  public:
		Window(int width, int height, const std::string& title);
		~Window();

		bool               Initialize();
		void               Update();
		[[nodiscard]] bool ShouldClose() const;
		void               SwapBuffers() const;
		void               PollEvents() const;
		void               Shutdown();
		void               OnResize(int width, int height);

		[[nodiscard]] int GetWidth() const;
		[[nodiscard]] int GetHeight() const;

		GLFWwindow* GetNativeWindow() const { return m_window; }

	  private:
		GLFWwindow* m_window;
		int         m_width;
		int         m_height;
		std::string m_title;
		bool        m_initialized;

		bool InitGLFW();
		bool InitGLAD();
		bool InitImGui();
	};
} // namespace Engine