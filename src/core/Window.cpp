#include "Window.h"

#include "core/Engine.h"

#include <glad/glad.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <spdlog/spdlog.h>

namespace Engine {
	Window::Window(int width, int height, const std::string& title) : m_window(nullptr), m_width(width), m_height(height), m_title(title), m_initialized(false)
	{
	}

	Window::~Window()
	{
		if (m_initialized) {
			Shutdown();
		}
	}

	bool Window::Initialize()
	{
		if (!InitGLFW()) return false;
		if (!InitGLAD()) return false;
		if (!InitImGui()) return false;

		m_initialized = true;
		return true;
	}

	bool Window::InitGLFW()
	{
		if (!glfwInit()) {
			spdlog::error("Failed to initialize GLFW");
			return false;
		}

		// Set window hints to allow resizing
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

		m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
		if (!m_window) {
			spdlog::error("Failed to create GLFW window");
			glfwTerminate();
			return false;
		}

		// Set resize callback
		glfwSetWindowSizeCallback(m_window, [](GLFWwindow* window, int width, int height) {
			Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
			if (win) win->OnResize(width, height);
		});

		// Store this pointer for the callback
		glfwSetWindowUserPointer(m_window, this);

		glfwMakeContextCurrent(m_window);
		return true;
	}

	bool Window::InitGLAD()
	{
		if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
			spdlog::error("Failed to initialize GLAD");
			glfwTerminate();
			return false;
		}
		return true;
	}

	bool Window::InitImGui()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		ImGui_ImplGlfw_InitForOpenGL(m_window, true);
		ImGui_ImplOpenGL3_Init("#version 150");

		ImGui::StyleColorsDark();
		io.FontGlobalScale = 2.0f;
		return true;
	}

	void Window::Update()
	{
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	bool Window::ShouldClose() const
	{
		return glfwWindowShouldClose(m_window);
	}

	void Window::SwapBuffers() const
	{
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(m_window);
	}

	void Window::PollEvents() const
	{
		glfwPollEvents();
	}

	void Window::Shutdown()
	{
		spdlog::info("Shutting down ImGui context");
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		// Terminate GLFW
		spdlog::info("Shutting down glfw window");
		glfwTerminate();
	}

	int Window::GetWidth() const
	{
		return m_width;
	}

	int Window::GetHeight() const
	{
		return m_height;
	}

	void Window::OnResize(int width, int height)
	{
		m_width  = width;
		m_height = height;

		// Update viewport
		glViewport(0, 0, width, height);
	}
} // namespace Engine