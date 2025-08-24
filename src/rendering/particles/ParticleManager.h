#pragma once

#include "Camera.h"
#include "core/Window.h"

#include <Effekseer/Effekseer.h>
#include <EffekseerRendererGL/EffekseerRendererGL.h>
#include <entt/entt.hpp>
#include <memory>
#include <string>
namespace Engine {

	class ParticleManager : public Module {
	  public:
		void        onInit() override;
		void        onUpdate(float dt) override;
		void        onGameStart() override {}
		void        onShutdown() override;
		std::string name() const override { return "ParticleModule"; };
		void        StopAllEffects();

		void Render();

		Effekseer::Handle            PlayEffect(const std::u16string& path, float x, float y, float z);
		const Effekseer::ManagerRef& GetManager() const { return m_manager; }

	  private:
		class DebugTextureLoader;

		EffekseerRendererGL::RendererRef m_renderer;
		Effekseer::ManagerRef            m_manager;
	};

} // namespace Engine
