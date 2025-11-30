#pragma once
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <memory>
#include <string>
#include <map>
#include "rendering/Framebuffer.h"
#include "core/module/Module.h"

namespace Engine {
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

	};
} // namespace Engine