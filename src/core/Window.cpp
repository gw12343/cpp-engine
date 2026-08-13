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

	// Only the icons actually referenced by editor UI / COMPONENT_LIST.
	// Loading ICON_MIN_FA..ICON_MAX_FA bloats the font atlas to thousands of glyphs
	// and makes ImGui_ImplOpenGL3_RenderDrawData much more expensive.
	static const char* const kUsedFaIcons[] = {
	    ICON_FA_SCROLL,
	    ICON_FA_MOON,
	    ICON_FA_MAXIMIZE,
	    ICON_FA_MAP,
	    ICON_FA_CUBE,
	    ICON_FA_CUBES_STACKED,
	    ICON_FA_VOLUME_HIGH,
	    ICON_FA_STAR_HALF_STROKE,
	    ICON_FA_WINDOW_MAXIMIZE,
	    ICON_FA_GLOBE,
	    ICON_FA_FONT,
	    ICON_FA_PLAY,
	    ICON_FA_PAUSE,
	    ICON_FA_STOP,
	    ICON_FA_TRASH,
	    ICON_FA_PALETTE,
	    ICON_FA_MAGNIFYING_GLASS,
	    ICON_FA_ROTATE,
	    ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT,
	    ICON_FA_BORDER_TOP_LEFT,
	    ICON_FA_UP_RIGHT_AND_DOWN_LEFT_FROM_CENTER,
	};

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

		// Set resize callback
		glfwSetWindowSizeCallback(m_window, [](GLFWwindow* window, int width, int height) {
			auto* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
			if (win) win->OnResize(width, height);
		});

		// Store this pointer for the callback
		glfwSetWindowUserPointer(m_window, this);

		glfwMakeContextCurrent(m_window);
		glfwSwapInterval(0);

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
		// Slightly cheaper docking/nav defaults
		io.ConfigWindowsResizeFromEdges = true;
		io.ConfigMemoryCompactTimer     = 60.0f;

		ImGui_ImplGlfw_InitForOpenGL(m_window, true);
		// Desktop GL 3.3 core path (VtxOffset for large meshes, less legacy state).
		ImGui_ImplOpenGL3_Init("#version 330");

		ImGui::StyleColorsDark();
		// AA lines/fills inflate vertex counts for every window chrome draw.
		ImGuiStyle& style       = ImGui::GetStyle();
		style.AntiAliasedLines  = false;
		style.AntiAliasedFill   = false;
		style.AntiAliasedLinesUseTex = false;

		// Load Roboto
		ImFontConfig roboto_config;
		roboto_config.MergeMode  = false;
		roboto_config.PixelSnapH = true;
		// Lower oversampling → smaller atlas, faster UI text batches.
		roboto_config.OversampleH = 1;
		roboto_config.OversampleV = 1;
		io.Fonts->AddFontFromFileTTF("resources/fonts/Roboto-Regular.ttf", 18.0f, &roboto_config, io.Fonts->GetGlyphRangesDefault());

		// Merge only FA icons we actually use (not the entire 0xe005..0xf8ff plane).
		static ImVector<ImWchar> faRanges;
		{
			ImFontGlyphRangesBuilder builder;
			for (const char* icon : kUsedFaIcons) {
				builder.AddText(icon);
			}
			builder.BuildRanges(&faRanges);
		}
		ImFontConfig fa_config;
		fa_config.MergeMode        = true;
		fa_config.PixelSnapH       = true;
		fa_config.GlyphMinAdvanceX = 12.0f;
		fa_config.OversampleH      = 1;
		fa_config.OversampleV      = 1;
		io.Fonts->AddFontFromFileTTF("resources/fonts/fa-solid-900.ttf", 18.0f, &fa_config, faRanges.Data);

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
		ZoneScopedN("PostRender");

#ifndef GAME_BUILD
		{
			ZoneScopedN("ImGui::Render");
			// Pure CPU: finalize draw lists. GPU can still be finishing the game pass
			// (caller should glFlush before this when possible).
			ImGui::Render();
		}
		{
			ZoneScopedN("ImGui OpenGL Draw");
			ImDrawData* drawData = ImGui::GetDrawData();
			if (drawData && drawData->CmdListsCount > 0 && drawData->TotalVtxCount > 0) {
				// UI always composites onto the default framebuffer.
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				glViewport(0, 0, m_width, m_height);
				ImGui_ImplOpenGL3_RenderDrawData(drawData);
			}
		}
#endif

		{
			ZoneScopedN("glfwSwapBuffers");
			glfwSwapBuffers(m_window);
		}
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