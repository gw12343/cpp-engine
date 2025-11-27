#pragma once
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <memory>
#include <string>
#include <map>
#include "rendering/Framebuffer.h"
#include "core/module/Module.h"

// Forward declarations
namespace Rml {
	class Context;
}

namespace Engine {
	class RmlUi_RenderInterface_GL3;
	class RmlUi_SystemInterface;

	class Window : public Module {
	  public:
		enum class FramebufferID { GAME_OUT, MOUSE_PICKING };

		Window(int width, int height, std::string title);
		~Window();


		[[nodiscard]] std::string name() const override { return "WindowModule"; }
		void                      onInit() override;
		void                      onGameStart() override;
		void                      onUpdate(float dt) override;
		void                      onShutdown() override;
		void                      setLuaBindings() override;

		[[nodiscard]] bool ShouldClose() const;
		void               SwapBuffers() const;
		void               OnResize(int width, int height);
		void               SetFullViewport() const;

		[[nodiscard]] int                    GetWidth() const;
		[[nodiscard]] int                    GetHeight() const;
		[[maybe_unused]] [[nodiscard]] float GetAspectRatio() const;
		[[nodiscard]] float                  GetTargetAspectRatio();

		GLFWwindow* GetNativeWindow() const { return m_window; }
		void        UpdateViewportSize(int render_width, int render_height, int x, int y);

		static void                         UpdateFramebufferSizes(int render_width, int render_height);
		static std::shared_ptr<Framebuffer> GetFramebuffer(FramebufferID id);

		static std::map<FramebufferID, std::shared_ptr<Framebuffer>> m_frameBuffers;

		// RmlUi access
		Rml::Context* GetRmlContext() const { return m_rmlContext; }
		RmlUi_RenderInterface_GL3* GetRmlRenderer() const { return m_rmlRenderer.get(); }

		int targetWidth;
		int targetHeight;
		int targetX;
		int targetY;

	  private:
		GLFWwindow* m_window;
		int         m_width;
		int         m_height;
		std::string m_title;

		bool        InitGLFW();
		static bool InitGLAD();
		bool        InitImGui();
		bool        InitRmlUi();

		// RmlUi
		Rml::Context*                                 m_rmlContext = nullptr;
		std::unique_ptr<RmlUi_RenderInterface_GL3>    m_rmlRenderer;
		std::unique_ptr<RmlUi_SystemInterface>        m_rmlSystem;
		bool m_rmlLuaInitialized = false;
	};
} // namespace Engine