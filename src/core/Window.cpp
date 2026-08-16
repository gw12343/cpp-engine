#include "Window.h"
#include "EngineData.h"
#include "Input.h"
#include "rendering/ui/GameUIManager.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <imgui.h>
#include <spdlog/spdlog.h>
#include <cfloat>
#include <utility>

#include "scripting/ScriptManager.h"
#include "rendering/ui/IconsFontAwesome6.h"
#include "imguizmo/ImGuizmo.h"
#include "imguizmo/ImGuizmo.h"
#include "rendering/Renderer.h"

#ifndef GAME_BUILD
#include <stb/stb_image.h>
#endif
namespace Engine {

	namespace {
		// Tracks whether we (play-mode capture) currently own the ImGui block flags,
		// so we don't clobber editor RMB fly-cam's NoMouse handling in Camera.
		bool g_imguiBlockedForPlayCapture = false;

		void UpdateImGuiPlayModeInputBlock()
		{
			ImGuiIO& io = ImGui::GetIO();

			// Scripts set the game cursor mode before Window::onUpdate (ScriptManager runs first).
			// Hold Escape to free the cursor and let ImGui take mouse/keyboard/gamepad
			// (player scripts re-assert CURSOR_DISABLED every frame, so we must key off Escape here).
			const bool playing     = GetState() == PLAYING;
			const bool escapeHeld  = GetInput().IsKeyPressed(GLFW_KEY_ESCAPE);
			const bool captured    = GetInput().GetCursorModeGame() == GLFW_CURSOR_DISABLED;
			const bool wantBlock   = playing && captured && !escapeHeld;

			if (wantBlock) {
				io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
				io.ConfigFlags |= ImGuiConfigFlags_NoKeyboard;
				io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
				io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableGamepad;

				// Drop residual events so held WASD / sticks don't drive UI next frame.
				if (!g_imguiBlockedForPlayCapture) {
					io.ClearInputKeys();
					io.ClearInputMouse();
				}
				g_imguiBlockedForPlayCapture = true;
			}
			else if (g_imguiBlockedForPlayCapture) {
				io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
				io.ConfigFlags &= ~ImGuiConfigFlags_NoKeyboard;
				io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
				io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
				io.ClearInputKeys();
				io.ClearInputMouse();
				g_imguiBlockedForPlayCapture = false;
			}
		}
	} // namespace

	static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};

	std::map<Window::FramebufferID, std::shared_ptr<Framebuffer>> Window::m_frameBuffers;
    std::shared_ptr<GBuffer> Window::m_gbuffer;
    std::shared_ptr<SSAOBuffer> Window::m_ssaobuffer;


	Window::Window(int width, int height, std::string title) : m_window(nullptr), m_width(width), m_height(height), m_title(std::move(title))
	{
		m_frameBuffers[Window::FramebufferID::LIGHTING]      = std::make_shared<Framebuffer>(GL_LINEAR, GL_LINEAR, true);
		m_frameBuffers[Window::FramebufferID::GAME_OUT]      = std::make_shared<Framebuffer>(GL_LINEAR, GL_LINEAR, true);
		m_frameBuffers[Window::FramebufferID::MOUSE_PICKING] = std::make_shared<Framebuffer>(GL_NEAREST, GL_NEAREST, false);
        m_gbuffer = std::make_shared<GBuffer>();
        m_ssaobuffer = std::make_shared<SSAOBuffer>();
	}

	// Explicit destructor needed for unique_ptr with forward-declared types
	Window::~Window() = default;


#ifndef GAME_BUILD
    void APIENTRY GLDebugCallback(
            GLenum source,
            GLenum type,
            GLuint id,
            GLenum severity,
            GLsizei length,
            const GLchar* message,
            const void* userParam)
    {
        GetWindow().log->error("GL DEBUG: {}\n", message);
    }
#endif

	void Window::onInit()
	{
        ZoneScopedN("Initialize Window");
		targetWidth  = 800;
		targetHeight = 600;

		InitGLFW();
		InitGLAD();
		InitImGui();

        // Debug Support — async messages only. SYNCHRONOUS forces a CPU/GPU pipeline
        // stall around GL calls and tanks editor framerate for little benefit day-to-day.
#ifndef GAME_BUILD
        glEnable(GL_DEBUG_OUTPUT);
        // glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // enable only when debugging GL errors
        glDebugMessageCallback(GLDebugCallback, nullptr);

        glDebugMessageControl(
                GL_DONT_CARE,
                GL_DONT_CARE,
                GL_DEBUG_SEVERITY_NOTIFICATION,
                0, nullptr,
                GL_FALSE
        );
#endif

        m_gbuffer->Init(m_width, m_height);

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
		glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

		m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
		if (!m_window) {
			spdlog::error("Failed to create GLFW window");
			glfwTerminate();
			return false;
		}

#ifndef GAME_BUILD
		// Editor window icon (taskbar / title bar on Windows)
		{
			int iconW = 0, iconH = 0, iconChannels = 0;
			unsigned char* iconPixels = stbi_load("resources/engine/icon.png", &iconW, &iconH, &iconChannels, 4);
			if (iconPixels) {
				GLFWimage icon{};
				icon.width  = iconW;
				icon.height = iconH;
				icon.pixels = iconPixels;
				glfwSetWindowIcon(m_window, 1, &icon);
				stbi_image_free(iconPixels);
			}
			else {
				spdlog::warn("Failed to load window icon: resources/engine/icon.png");
			}
		}
#endif

		// Set resize callback
		glfwSetWindowSizeCallback(m_window, [](GLFWwindow* window, int width, int height) {
			auto* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
			if (win) win->OnResize(width, height);
		});

		// Store this pointer for the callback
		glfwSetWindowUserPointer(m_window, this);

		glfwMakeContextCurrent(m_window);
		glfwSwapInterval(1);

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

		// Install after ImGui so we own the drop callback (ImGui does not use it).
		glfwSetDropCallback(m_window, [](GLFWwindow* window, int count, const char** paths) {
			auto* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
			if (win) win->OnFilesDropped(count, paths);
		});

		return true;
	}

	void Window::OnFilesDropped(int count, const char** paths)
	{
		if (!paths || count <= 0) return;
		m_droppedFiles.clear();
		m_droppedFiles.reserve(static_cast<size_t>(count));
		for (int i = 0; i < count; ++i) {
			if (paths[i] && paths[i][0] != '\0') {
				m_droppedFiles.emplace_back(paths[i]);
			}
		}
		m_dropAgeFrames = 0;
		if (!m_droppedFiles.empty()) {
			log->info("OS file drop: {} path(s)", m_droppedFiles.size());
		}
	}

	std::vector<std::string> Window::ConsumeDroppedFiles()
	{
		std::vector<std::string> out;
		out.swap(m_droppedFiles);
		m_dropAgeFrames = 0;
		return out;
	}

	void Window::onUpdate(float dt)
	{
		ZoneScoped;
		glfwPollEvents();

		// Age out unclaimed OS drops so they don't stick forever.
		if (!m_droppedFiles.empty()) {
			++m_dropAgeFrames;
			if (m_dropAgeFrames > 2) {
				m_droppedFiles.clear();
				m_dropAgeFrames = 0;
			}
		}

		// While playing with a captured cursor, keep mouse/keyboard/gamepad off ImGui
		// so editor panels don't steal WASD, look, or pad navigation.
		UpdateImGuiPlayModeInputBlock();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		// Backend may have re-injected events this frame — wipe them while blocked.
		if (g_imguiBlockedForPlayCapture) {
			ImGuiIO& io = ImGui::GetIO();
			io.ClearInputKeys();
			io.ClearInputMouse();
			io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
		}
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();

#ifdef GAME_BUILD
		targetX      = 0;
		targetY      = 0;
		targetWidth  = GetWidth();
		targetHeight = GetHeight();
#endif
	}

void Window::onGameStart()
{
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

		for (auto& [id, fb] : m_frameBuffers) {
			if (fb) {
				fb->Delete();
			}
		}
		m_frameBuffers.clear();
		if (m_gbuffer) {
			m_gbuffer->Delete();
			m_gbuffer.reset();
		}
		if (m_ssaobuffer) {
			m_ssaobuffer->Delete();
			m_ssaobuffer.reset();
		}

		if (m_window) {
			glfwDestroyWindow(m_window);
			m_window = nullptr;
		}

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
        if(width < 10) width = 10;
        if(height < 10) height = 10;

		m_width  = width;
		m_height = height;

		// Update viewport
		SetFullViewport();
		UpdateFramebufferSizes(width, height);

		// Update RmlUi context dimensions
		GetGameUIManager().OnResize(width, height);
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
        m_gbuffer->Resize(render_width, render_height);
        m_ssaobuffer->Resize(render_width, render_height, false);
	}


	void Window::UpdateViewportSize(int render_width, int render_height, int x, int y)
	{
		targetX = x;
		targetY = y;
		if (render_width != targetWidth || render_height != targetHeight) {
			targetWidth  = render_width;
            if(targetWidth < 10) targetWidth = 10;
			targetHeight = render_height;
            if(targetHeight < 10) targetHeight = 10;
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