#include "Window.h"
#include "EngineData.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <glad/glad.h>
#include <imgui.h>
#include <spdlog/spdlog.h>
#include <utility>
#include <tracy/Tracy.hpp>
#include "scripting/ScriptManager.h"
#include "rendering/ui/IconsFontAwesome6.h"
#include "imguizmo/ImGuizmo.h"
#include <RmlUi/Core.h>
#include <RmlUi/Lua.h>
#include "rendering/ui/RmlUiBackend.h"

namespace Engine {

	static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};

	std::map<Window::FramebufferID, std::shared_ptr<Framebuffer>> Window::m_frameBuffers;

	Window::Window(int width, int height, std::string title) : m_window(nullptr), m_width(width), m_height(height), m_title(std::move(title))
	{
		m_frameBuffers[Window::FramebufferID::GAME_OUT]      = std::make_shared<Framebuffer>(GL_LINEAR, GL_LINEAR);
		m_frameBuffers[Window::FramebufferID::MOUSE_PICKING] = std::make_shared<Framebuffer>(GL_NEAREST, GL_NEAREST);
	}

	// Explicit destructor needed for unique_ptr with forward-declared types
	Window::~Window() = default;


	void Window::onInit()
	{
		targetWidth  = 800;
		targetHeight = 600;

		InitGLFW();
		InitGLAD();
		InitImGui();
		InitRmlUi();

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

	bool Window::InitRmlUi()
	{
		// Create system and render interfaces
		m_rmlSystem = std::make_unique<RmlUi_SystemInterface>();
		m_rmlRenderer = std::make_unique<RmlUi_RenderInterface_GL3>();

		// Install interfaces
		Rml::SetSystemInterface(m_rmlSystem.get());
		Rml::SetRenderInterface(m_rmlRenderer.get());

		// Initialize RmlUi
	if (!Rml::Initialise()) {
		spdlog::error("Failed to initialize RmlUi");
		return false;
	}

	// Note: RmlUi Lua plugin will be initialized later in onUpdate
	// after ScriptManager is ready

	// Create context
	m_rmlContext = Rml::CreateContext("main", Rml::Vector2i(m_width, m_height));
		if (!m_rmlContext) {
			spdlog::error("Failed to create RmlUi context");
			return false;
		}

		// Set viewport
		m_rmlRenderer->SetViewport(m_width, m_height);

		// Load fonts
		if (!Rml::LoadFontFace("resources/fonts/Roboto-Regular.ttf")) {
			spdlog::warn("Failed to load Roboto font for RmlUi");
		}

		spdlog::info("RmlUi initialized successfully");
		return true;
	}

	void Window::onUpdate(float dt)
	{
		ZoneScoped;
		glfwPollEvents();

		// Initialize RmlUi Lua plugin once ScriptManager is ready
		// (Can't do this in onInit because ScriptManager isn't initialized yet)
		if (!m_rmlLuaInitialized && m_rmlContext) {
			try {
				lua_State* L = GetScriptManager().lua.lua_state();
				Rml::Lua::Initialise(L);
				m_rmlLuaInitialized = true;
				spdlog::info("RmlUi Lua plugin initialized with engine Lua state");
				
				// Load demo document now that Lua is ready to process scripts
				auto* document = m_rmlContext->LoadDocument("resources/ui/demo.rml");
				if (document) {
					document->Show();
					spdlog::info("Loaded RmlUi demo document with Lua scripting");
				} else {
					spdlog::warn("Failed to load RmlUi demo document");
				}
			} catch (const std::exception& e) {
				spdlog::error("Failed to initialize RmlUi Lua plugin: {}", e.what());
			}
		}

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

void Window::onGameStart()
{
	// Reload RmlUi document when play button is pressed (similar to script reloading)
	if (m_rmlContext && m_rmlLuaInitialized) {
		spdlog::info("Reloading RmlUi document...");
		
		// Close all existing documents to clean up old Lua subscriptions
		while (m_rmlContext->GetNumDocuments() > 0) {
			auto* doc = m_rmlContext->GetDocument(0);
			if (doc) {
				doc->Close();
			}
		}
		
		// Force stylesheet reload by clearing the cache
		// This ensures .rcss files are reloaded from disk
		Rml::Factory::ClearStyleSheetCache();
		
		// Reload the document - this will re-execute the Lua script and set up fresh subscriptions
		auto* document = m_rmlContext->LoadDocument("resources/ui/demo.rml");
		if (document) {
			document->Show();
			spdlog::info("Reloaded RmlUi demo document");
		} else {
			spdlog::warn("Failed to reload RmlUi demo document");
		}
	}
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

		// Shutdown RmlUi
	log->info("Shutting down RmlUi");
	if (m_rmlContext) {
		// In RmlUi 6.0, contexts are managed by Rml::Shutdown()
		m_rmlContext = nullptr;
	}
	Rml::Shutdown();
	m_rmlRenderer.reset();
	m_rmlSystem.reset();

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

		// Update RmlUi context dimensions
		if (m_rmlContext) {
			m_rmlContext->SetDimensions(Rml::Vector2i(width, height));
			m_rmlRenderer->SetViewport(width, height);
		}
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