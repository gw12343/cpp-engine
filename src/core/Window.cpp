#include "Window.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <glad/glad.h>
#include <imgui.h>
#include <spdlog/spdlog.h>
#include <utility>

namespace Engine {
	int Window::targetWidth  = 800;
	int Window::targetHeight = 600;
	int Window::targetX      = 0;
	int Window::targetY      = 0;

	std::map<Window::FramebufferID, std::shared_ptr<Framebuffer>> Window::m_frameBuffers;

	Window::Window(int width, int height, std::string title) : m_window(nullptr), m_width(width), m_height(height), m_title(std::move(title))
	{
	}

	Window::~Window() = default;

	bool Window::Initialize()
	{
		if (!InitGLFW()) return false;
		if (!InitGLAD()) return false;
		InitImGui();

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
			auto* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
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
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking
		ImGui_ImplGlfw_InitForOpenGL(m_window, true);
		ImGui_ImplOpenGL3_Init("#version 150");

		ImGui::StyleColorsDark();
		io.FontGlobalScale = 1.0f;
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

	void Window::PollEvents()
	{
		glfwPollEvents();
	}

	void Window::SetFullViewport() const
	{
		glViewport(0, 0, m_width, m_height);
	}

	void Window::Shutdown()
	{
		SPDLOG_INFO("Shutting down ImGui context");
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		// Terminate GLFW
		SPDLOG_INFO("Shutting down glfw window");
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
		SetFullViewport();
		UpdateFramebufferSizes(width, height);
	}
	[[maybe_unused]] float Window::GetAspectRatio() const
	{
		return static_cast<float>(m_width) / static_cast<float>(m_height);
	}

	float Window::GetTargetAspectRatio()
	{
		return static_cast<float>(targetWidth) / static_cast<float>(targetHeight);
	}

	void Window::UpdateFramebufferSizes(int render_width, int render_height)
	{
		for (const auto& [id, fb] : m_frameBuffers) {
			SPDLOG_DEBUG("Resizing framebuffer {}, new size: ({}, {})", id, render_width, render_height);
			fb->Resize(render_width, render_height);
		}
	}
	void Window::UpdateViewportSize(int render_width, int render_height, int x, int y)
	{
		targetX = x;
		targetY = y;
		if (render_width != targetWidth || render_height != targetHeight) {
			targetWidth  = render_width;
			targetHeight = render_height;
		}
	}
	std::shared_ptr<Framebuffer> Window::GetFramebuffer(Window::FramebufferID id)
	{
		return m_frameBuffers[id];
	}
} // namespace Engine