#pragma once

#include "core/module/Module.h"
#include <RmlUi/Core.h>
#include <RmlUi/Lua.h>
#include "rendering/ui/RmlUiBackend.h"
#include <memory>

namespace Engine {

	class GameUIManager : public Module {
	public:
		GameUIManager();
		~GameUIManager() override;

		void onInit() override;
		void onUpdate(float dt) override;
		void onGameStart() override;
		void onShutdown() override;
		void resetDocuments();
		[[nodiscard]] std::string name() const override { return "GameUIManager"; }

		void Render();
		void OnResize(int width, int height);

		Rml::Context* GetContext() const { return m_rmlContext; }

	private:
		std::unique_ptr<RmlUi_SystemInterface> m_rmlSystem;
		std::unique_ptr<RmlUi_RenderInterface_GL3> m_rmlRenderer;
		Rml::Context* m_rmlContext = nullptr;
		bool m_rmlLuaInitialized = false;
	};

} // namespace Engine
