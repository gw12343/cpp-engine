#pragma once

#include "Camera.h"
#include "core/Window.h"

#include <Effekseer/Effekseer.h>
#include <EffekseerRendererGL/EffekseerRendererGL.h>
#include <entt/entt.hpp>
#include <memory>
#include <string>
namespace Engine {

	class ParticleManager {
	  public:
		ParticleManager();
		~ParticleManager();

		bool Initialize(int maxInstances = 8000);
		void Update(entt::registry& registry, float deltaTime);
		void Render(Window& window, Camera& camera);
		void Shutdown();

		Effekseer::Handle            PlayEffect(const std::u16string& path, float x, float y, float z);
		const Effekseer::ManagerRef& GetManager() const { return m_manager; }

	  private:
		class DebugTextureLoader;

		EffekseerRendererGL::RendererRef m_renderer;
		Effekseer::ManagerRef            m_manager;
	};

} // namespace Engine
