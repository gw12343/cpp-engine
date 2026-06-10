#pragma once

#include "core/Window.h"

#include <Effekseer/Effekseer.h>
#include <EffekseerRendererGL/EffekseerRendererGL.h>
#include <core/Entity.h>


namespace Engine {

	class ParticleManager : public Module {
	  public:
		void                      onInit() override;
		void                      onUpdate(float dt) override;
		void                      onGameStart() override;
		void                      onShutdown() override;
		[[nodiscard]] std::string name() const override { return "ParticleModule"; };
		void                      ResetInternalManager();

		void Render();

		void                          PlayEffect(Entity& entity);
		[[nodiscard]] const Effekseer::ManagerRef& GetManager() const { return m_manager; }
        void        setLuaBindings() override;


	  private:
		class DebugTextureLoader;

		EffekseerRendererGL::RendererRef m_renderer;
		Effekseer::ManagerRef            m_manager;
	};

} // namespace Engine
