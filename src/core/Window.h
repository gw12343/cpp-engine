#pragma once
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>


#include <map>
#include <string>
#include <vector>
#include "rendering/Framebuffer.h"
#include "core/module/Module.h"
#include "rendering/effects/ssao/GBuffer.h"
#include "rendering/effects/ssao/SSAOBuffer.h"

namespace Engine {
	class Window : public Module {
	  public:
		enum class FramebufferID { LIGHTING, GAME_OUT, MOUSE_PICKING };

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

        static std::shared_ptr<GBuffer> m_gbuffer;
        static std::shared_ptr<SSAOBuffer> m_ssaobuffer;

        std::shared_ptr<GBuffer> GetGBuffer() { return m_gbuffer; }
        std::shared_ptr<SSAOBuffer> GetSSAOBuffer() { return m_ssaobuffer; }

		/** OS file drops (Explorer → window). Consumed by editor UI that wants them. */
		[[nodiscard]] bool                       HasDroppedFiles() const { return !m_droppedFiles.empty(); }
		std::vector<std::string>                 ConsumeDroppedFiles();
		void                                     ClearDroppedFiles() { m_droppedFiles.clear(); m_dropAgeFrames = 0; }

		int targetWidth;
		int targetHeight;
		int targetX;
		int targetY;

	  private:
		GLFWwindow*              m_window;
		int                      m_width;
		int                      m_height;
		std::string              m_title;
		std::vector<std::string> m_droppedFiles;
		int                      m_dropAgeFrames = 0;

		void        OnFilesDropped(int count, const char** paths);
		bool        InitGLFW();
		static bool InitGLAD();
		// Game builds still run an ImGui frame (NewFrame/Render in onUpdate/SwapBuffers).
		bool        InitImGui();

	};
} // namespace Engine