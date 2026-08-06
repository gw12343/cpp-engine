#pragma once

#include "core/Window.h"

#include <Effekseer/Effekseer.h>
#include <EffekseerRendererGL/EffekseerRendererGL.h>
#include <core/Entity.h>

#include <cstdint>

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

		void                                       PlayEffect(Entity& entity);
		[[nodiscard]] const Effekseer::ManagerRef& GetManager() const { return m_manager; }
		void                                       setLuaBindings() override;

		/// True if any effect instance is currently alive (cheap early-out for render/update).
		[[nodiscard]] bool HasActiveEffects() const;

	  private:
		class DebugTextureLoader;

		void SyncEffectTransforms();
		void RestoreGLStateAfterParticles();

		EffekseerRendererGL::RendererRef m_renderer;
		Effekseer::ManagerRef            m_manager;

		// Caps for instance pool / sprite GPU buffers (8000 was excessive; still enough for fireworks).
		static constexpr int kMaxInstances = 2048;
		static constexpr int kSquareMax    = 4096;
	};

} // namespace Engine
