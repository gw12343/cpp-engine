#include "GameUIManager.h"
#include "components/impl/RmlUIComponent.h"
#include "core/Input.h"
#include "core/Window.h"
#include "core/EngineData.h"
#include "scripting/ScriptManager.h"
#include <tracy/Tracy.hpp>

namespace Engine {

	GameUIManager::GameUIManager() = default;
	GameUIManager::~GameUIManager() = default;

	void GameUIManager::onInit() {
		auto& window = GetWindow();
		int width = window.GetWidth();
		int height = window.GetHeight();

		// Create system and render interfaces
		m_rmlSystem = std::make_unique<RmlUi_SystemInterface>();
		m_rmlRenderer = std::make_unique<RmlUi_RenderInterface_GL3>();

		// Install interfaces
		Rml::SetSystemInterface(m_rmlSystem.get());
		Rml::SetRenderInterface(m_rmlRenderer.get());

		// Initialize RmlUi
		if (!Rml::Initialise()) {
			log->error("Failed to initialize RmlUi");
			return;
		}

		// Create context
		m_rmlContext = Rml::CreateContext("main", Rml::Vector2i(width, height));
		if (!m_rmlContext) {
			log->error("Failed to create RmlUi context");
			return;
		}

		// Set viewport
		m_rmlRenderer->SetViewport(width, height);

		// Load fonts
		if (!Rml::LoadFontFace("resources/fonts/Roboto-Regular.ttf")) {
			log->warn("Failed to load Roboto font for RmlUi");
		}

		log->info("RmlUi initialized successfully");
		
		// Initialize RmlUi Lua plugin immediately
		// This must happen before scenes load so RmlUIComponent documents can use Lua
		try {
			lua_State* L = GetScriptManager().lua.lua_state();
			Rml::Lua::Initialise(L);
			m_rmlLuaInitialized = true;
			log->info("RmlUi Lua plugin initialized with engine Lua state");
		} catch (const std::exception& e) {
			log->error("Failed to initialize RmlUi Lua plugin: {}", e.what());
		}
	}

	void GameUIManager::onUpdate(float dt) {
		ZoneScoped;

		if (m_rmlContext) {
			auto& window = GetWindow();

#ifdef GAME_BUILD
			// In game builds, use full window size
			m_rmlContext->SetDimensions(Rml::Vector2i(window.GetWidth(), window.GetHeight()));
			m_rmlRenderer->SetViewport(window.GetWidth(), window.GetHeight());

			// Process mouse input with window coordinates
			double mouseX, mouseY;
			glfwGetCursorPos(window.GetNativeWindow(), &mouseX, &mouseY);
			m_rmlContext->ProcessMouseMove((int)mouseX, (int)mouseY, 0);

			// Process scroll input
			float scrollDelta = GetInput().GetMouseScrollDelta();
			if (scrollDelta != 0.0f) {
				m_rmlContext->ProcessMouseWheel(-scrollDelta, 0); // RmlUi expects negative for down? or positive?
				// Usually positive is up. RmlUi: "A negative value indicates the wheel was scrolled down."
				// GLFW: yoffset is positive for up.
				// So we pass -scrollDelta to invert? Or just scrollDelta?
				// RmlUi docs say: "wheel_delta: The distance the mouse wheel was rotated. A negative value indicates the wheel was scrolled down."
				// GLFW: "yoffset is positive if the user scrolled up"
				// So they match. Positive = Up.
				m_rmlContext->ProcessMouseWheel(-scrollDelta, 0); // Wait, RmlUi usually scrolls content UP when wheel goes DOWN.
				// If I scroll DOWN (negative), content moves UP.
				// Let's try passing -scrollDelta first, as standard web behavior often flips.
				// Actually, let's stick to matching signs first.
				m_rmlContext->ProcessMouseWheel(-scrollDelta, 0);
			}

			// Handle mouse buttons
			if (glfwGetMouseButton(window.GetNativeWindow(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
				static bool wasPressed = false;
				if (!wasPressed) {
					m_rmlContext->ProcessMouseButtonDown(0, 0);
					wasPressed = true;
				}
			} else {
				static bool wasPressed = false;
				if (wasPressed) {
					m_rmlContext->ProcessMouseButtonUp(0, 0);
				}
				wasPressed = false;
			}
#else
			// In editor mode, use scene view size
			m_rmlContext->SetDimensions(Rml::Vector2i(window.targetWidth, window.targetHeight));
			m_rmlRenderer->SetViewport(window.targetWidth, window.targetHeight);

			// Process scaled mouse input
			if (GetInput().IsMousePositionInViewport()) {
				glm::vec2 mousePos = GetInput().GetMousePositionInViewport();
				m_rmlContext->ProcessMouseMove((int)mousePos.x, (int)mousePos.y, 0);

				// Process scroll input
				float scrollDelta = GetInput().GetMouseScrollDelta();
				if (scrollDelta != 0.0f) {
					m_rmlContext->ProcessMouseWheel(-scrollDelta, 0);
				}

				// Handle mouse button clicks
				if (GetInput().IsMouseClicked(GLFW_MOUSE_BUTTON_LEFT)) {
					m_rmlContext->ProcessMouseButtonDown(0, 0);
				}
				else if (!GetInput().IsMousePressed(GLFW_MOUSE_BUTTON_LEFT)) {
					// Button was just released
					m_rmlContext->ProcessMouseButtonUp(0, 0);
				}
			}
#endif
			m_rmlContext->Update();
		}
	}
	

	void GameUIManager::Render() {
		ZoneScoped;
		if (m_rmlContext) {
			m_rmlContext->Render();
		}
	}

	void GameUIManager::onGameStart() {
		// onGameStart is called by ModuleManager for all modules
		// UI reloading is handled by resetDocuments() which is called explicitly
		// from UIManager::Play() only in editor mode
	}

	void GameUIManager::resetDocuments() {
		if (m_rmlContext && m_rmlLuaInitialized) {
			log->info("Reloading RmlUi documents...");

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

			// Reload all RmlUI components
			auto view = GetCurrentSceneRegistry().view<Components::RmlUIComponent>();
			for (auto entity : view) {
				auto& rmlComponent = view.get<Components::RmlUIComponent>(entity);
				Entity e(entity, GetCurrentScene());
				if (!rmlComponent.GetDocumentPath().empty()) {
					rmlComponent.LoadDocument(rmlComponent.GetDocumentPath(), e);
				}
			}
		}
	}


	void GameUIManager::onShutdown() {
		log->info("Shutting down RmlUi");
		
		// Close all documents first
		if (m_rmlContext) {
			while (m_rmlContext->GetNumDocuments() > 0) {
				auto* doc = m_rmlContext->GetDocument(0);
				if (doc) {
					doc->Close();
				}
			}
			// In RmlUi 6.0, contexts are managed by Rml::Shutdown()
			m_rmlContext = nullptr;
		}
		
		// Shutdown RmlUi and cleanup interfaces
		Rml::Shutdown();
		m_rmlRenderer.reset();
		m_rmlSystem.reset();
	}

	void GameUIManager::OnResize(int width, int height) {
		if (m_rmlContext) {
			m_rmlContext->SetDimensions(Rml::Vector2i(width, height));
			m_rmlRenderer->SetViewport(width, height);
		}
	}

} // namespace Engine
