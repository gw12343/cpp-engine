#include "Window.h"
#include "EngineData.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <glad/glad.h>
#include <imgui.h>
#include <spdlog/spdlog.h>
#include <utility>
#include "scripting/ScriptManager.h"
#include "rendering/ui/IconsFontAwesome6.h"
#include "imguizmo/ImGuizmo.h"

namespace Engine {

	static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};

	std::map<Window::FramebufferID, std::shared_ptr<Framebuffer>> Window::m_frameBuffers;

	Window::Window(int width, int height, std::string title) : m_window(nullptr), m_width(width), m_height(height), m_title(std::move(title))
	{
		m_frameBuffers[Window::FramebufferID::GAME_OUT]      = std::make_shared<Framebuffer>(FramebufferType::Standard, GL_LINEAR, GL_LINEAR);
		m_frameBuffers[Window::FramebufferID::MOUSE_PICKING] = std::make_shared<Framebuffer>(FramebufferType::Standard, GL_NEAREST, GL_NEAREST);
		m_frameBuffers[Window::FramebufferID::GBUFFER]       = std::make_shared<Framebuffer>(FramebufferType::GBuffer, GL_NEAREST, GL_NEAREST);
	}


	void Window::onInit()
	{
		targetWidth  = 800;
		targetHeight = 600;

		InitGLFW();
		InitGLAD();
		InitImGui();

		UpdateFramebufferSizes(m_width, m_height);
	}


	bool Window::InitGLFW()
	{
		if (!glfwInit()) {
			spdlog::error("Failed to initialize GLFW");
			return false;
		}

		// Set window hints to allow resizing
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
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

		// Load Roboto 12pt (approx. 24px)
		ImFontConfig roboto_config;
		roboto_config.MergeMode  = false;
		roboto_config.PixelSnapH = true;
		ImFont* roboto_font      = io.Fonts->AddFontFromFileTTF("resources/fonts/Roboto-Regular.ttf", 18.0f, &roboto_config, io.Fonts->GetGlyphRangesDefault());

		// Load Font Awesome and merge into Roboto
		ImFontConfig fa_config;
		fa_config.MergeMode        = true; // Important
		fa_config.PixelSnapH       = true;
		fa_config.GlyphMinAdvanceX = 12.0f; // Adjust icon spacing if needed
		io.Fonts->AddFontFromFileTTF("resources/fonts/fa-solid-900.ttf", 18.0f, &fa_config, icons_ranges);


		io.FontGlobalScale = 1.0f;
		ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());
		return true;
	}

	void Window::onUpdate(float dt)
	{
		glfwPollEvents();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();

#ifdef GAME_BUILD
		targetX      = 0;
		targetY      = 0;
		targetWidth  = GetWidth();
		targetHeight = GetHeight();
#endif
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


	void Window::SetFullViewport() const
	{
		glViewport(0, 0, m_width, m_height);
	}

	void Window::onShutdown()
	{
		log->info("Shutting down ImGui context");
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		// Terminate GLFW
		log->info("Shutting down glfw window");
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
	void Window::setLuaBindings()
	{
		// Bind the Window instance API
		GetScriptManager().lua.new_usertype<Window>("Window",
		                                            // Methods
		                                            "getWidth",
		                                            &Window::GetWidth,
		                                            "getHeight",
		                                            &Window::GetHeight,
		                                            "getAspectRatio",
		                                            &Window::GetAspectRatio,
		                                            "getTargetAspectRatio",
		                                            &Window::GetTargetAspectRatio,
		                                            "updateViewportSize",
		                                            &Window::UpdateViewportSize,

		                                            // Members (mutable instance variables)
		                                            "targetWidth",
		                                            &Window::targetWidth,
		                                            "targetHeight",
		                                            &Window::targetHeight,
		                                            "targetX",
		                                            &Window::targetX,
		                                            "targetY",
		                                            &Window::targetY);

		// Provide access to the main window
		GetScriptManager().lua.set_function("getWindow", []() -> Window& { return Engine::GetWindow(); });
	}


} // namespace Engine