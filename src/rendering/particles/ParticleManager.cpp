
#include "ParticleManager.h"


#include "core/EngineData.h"
#include "components/impl/TransformComponent.h"
#include "components/impl/ParticleSystemComponent.h"

#ifdef AddJob
#undef AddJob
#endif
#include <Camera.h>

#include "physics/PhysicsManager.h"


#include "scripting/ScriptManager.h"
#include "assets/AssetManager.h"

namespace Engine {
	static const int MAX_INSTANCES = 8000;

	class ParticleManager::DebugTextureLoader : public Effekseer::TextureLoader {
	  public:
		explicit DebugTextureLoader(const EffekseerRendererGL::RendererRef& renderer) : m_renderer(renderer), m_internalLoader(renderer->CreateTextureLoader()) {}

		Effekseer::TextureRef Load(const EFK_CHAR* path, Effekseer::TextureType type) override
		{
			std::u16string u16Path(path);
			std::string    pathStr(u16Path.begin(), u16Path.end());
			GetParticleManager().log->debug("Effekseer trying to load texture: {}   (type: {})", pathStr, (type == Effekseer::TextureType::Normal ? "NORMAL" : (type == Effekseer::TextureType::Distortion ? "DISTORTION" : "COLOR")));

			auto tex = m_internalLoader->Load(path, type);
			if (!tex) {
				GetParticleManager().log->error("Effekseer Failed to load texture: {}", pathStr);
			}

			return tex;
		}

		void Unload(Effekseer::TextureRef data) override { m_internalLoader->Unload(data); }

	  private:
		EffekseerRendererGL::RendererRef m_renderer;
		Effekseer::TextureLoaderRef      m_internalLoader;
	};

    void ParticleManager::setLuaBindings() {
        // Bind the PhysicsManager class
        GetScriptManager().lua.new_usertype<ParticleManager>("ParticleManager",
                                                             "playEffect",
                                                             &ParticleManager::PlayEffect
        );

        // Provide access to the main PhysicsManager
        GetScriptManager().lua.set_function("getParticleManager", []() -> ParticleManager & { return Engine::GetParticleManager(); });
    }

	void ParticleManager::onInit()
	{
        ZoneScopedN("Initialize ParticleManager");
		m_renderer = EffekseerRendererGL::Renderer::Create(MAX_INSTANCES, EffekseerRendererGL::OpenGLDeviceType::OpenGL3);
		if (!m_renderer) {
			log->critical("Failed to create Effekseer renderer");
			return;
		}

		ResetInternalManager();
	}


	void ParticleManager::onGameStart(){
        auto view = GetCurrentSceneRegistry().view<Components::Transform, Components::ParticleSystem>();
        for (auto [entity, transform, particleSystem] : view.each()) {
            if (particleSystem.autoPlay && particleSystem.effect.IsValid()) {
                Particle* particle = GetAssetManager().Get(particleSystem.effect);
                if(!particle){
                    GetParticleManager().log->error("Particle effect is invalid!");
                    return;
                }

                // Get particle manager
                const auto& manager = GetParticleManager().GetManager();

                // Spawn particle system at transform
                auto pos       = transform.GetWorldPosition();
                particleSystem.handle         = manager->Play(particle->GetEffect(), pos.x, pos.y, pos.z);
            }
        }

    }


	void ParticleManager::onUpdate(float dt)
	{
		ZoneScoped;
		if (GetState() != PLAYING) return;

		if (m_manager) {
			m_manager->Update(dt * 60.0f);
		}

		// Update particles systems' locations
		auto view = GetCurrentSceneRegistry().view<Components::Transform, Components::ParticleSystem>();
		for (auto [entity, transform, particleSystem] : view.each()) {
			auto pos = transform.GetWorldPosition();

            if(particleSystem.looping && !m_manager->Exists(particleSystem.handle)){
                Particle* particle = GetAssetManager().Get(particleSystem.effect);
                if(particle) {
                    particleSystem.handle = m_manager->Play(particle->GetEffect(), pos.x, pos.y, pos.z);
                }
            }
			m_manager->SetLocation(particleSystem.handle, pos.x, pos.y, pos.z);
		}
	}


	void ParticleManager::onShutdown()
	{
		m_manager.Reset();
		m_renderer.Reset();
	}


	void ParticleManager::Render()
	{
		// Set up OpenGL state for Effekseer
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		//glEnable(GL_CULL_FACE);
		glDisable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glDepthMask(GL_FALSE);
		if (m_renderer) {
			::Effekseer::Matrix44 projMat = ConvertGLMToEffekseerMatrix(GetCamera().GetProjectionMatrix());
			::Effekseer::Matrix44 viewMat = ConvertGLMToEffekseerMatrix(GetCamera().GetViewMatrix());


			m_renderer->SetProjectionMatrix(projMat);
			m_renderer->SetCameraMatrix(viewMat);

			m_renderer->BeginRendering();
			m_manager->Draw();
			m_renderer->EndRendering();
		}
		glDepthMask(GL_TRUE);
	}


//	Effekseer::Handle ParticleManager::PlayEffect(const std::u16string& path, float x, float y, float z)
//	{
//		Effekseer::RefPtr<Effekseer::Effect> effect = Effekseer::Effect::Create(m_manager, path.c_str());
//		if (!effect) {
//			log->critical("Failed to load particle effect: {}", std::string(path.begin(), path.end()));
//			return -1;
//		}
//
//		return m_manager->Play(effect, x, y, z);
//	}
	void ParticleManager::ResetInternalManager()
	{
		// Effekseer Effects are bound to a Manager instance. Creating a new manager
		// invalidates all previously loaded Effect refs — reload every Particle asset.
		m_manager = Effekseer::Manager::Create(MAX_INSTANCES);
		m_manager->SetSpriteRenderer(m_renderer->CreateSpriteRenderer());
		m_manager->SetRibbonRenderer(m_renderer->CreateRibbonRenderer());
		m_manager->SetRingRenderer(m_renderer->CreateRingRenderer());
		m_manager->SetTrackRenderer(m_renderer->CreateTrackRenderer());
		m_manager->SetModelRenderer(m_renderer->CreateModelRenderer());

		auto textureLoader = Effekseer::MakeRefPtr<DebugTextureLoader>(m_renderer);
		m_manager->SetTextureLoader(textureLoader);

		if (!m_manager) {
			log->error("Effekseer Manager not initialized!");
			return;
		}
		log->info("Effekseer Manager initialized!");

		// Clear instance handles — old handles are meaningless on the new manager.
		if (GetCurrentScene()) {
			auto view = GetCurrentSceneRegistry().view<Components::ParticleSystem>();
			for (auto [entity, ps] : view.each()) {
				ps.handle = -1;
			}
		}

		// Re-create Effect objects against the new manager.
		auto& storage = GetAssetManager().GetStorage<Particle>();
		int   reloaded = 0;
		for (auto& [guid, asset] : storage.guidToAsset) {
			if (!asset) continue;
			const std::string& path = asset->GetPath();
			if (path.empty()) {
				log->warn("Particle {} has empty path; cannot rebind to new manager", guid);
				continue;
			}
			if (!asset->LoadFromFile(path)) {
				log->error("Failed to reload particle effect after manager reset: {}", path);
				continue;
			}
			++reloaded;
		}
		if (reloaded > 0) {
			log->info("Rebound {} particle effect(s) to new Effekseer manager", reloaded);
		}
	}

	void ParticleManager::PlayEffect(Entity& entity)
	{
		const auto& manager = GetManager();
		ENGINE_VERIFY(manager != nullptr, "ParticleManager::PlayEffect: Effekseer manager is null");

		if (!entity.HasComponent<Components::ParticleSystem>()) {
			log->warn("Entity has no ParticleSystem; cannot play particle effect.");
			return;
		}

		if (!entity.HasComponent<Components::Transform>()) {
			log->warn("Entity has no Transform; cannot play particle effect.");
			return;
		}

		auto& ps  = entity.GetComponent<Components::ParticleSystem>();
		auto& tr  = entity.GetComponent<Components::Transform>();
		auto  pos = tr.GetWorldPosition();

		if (!ps.effect.IsValid()) {
			log->warn("ParticleSystem on '{}' has empty effect handle", entity.GetName());
			return;
		}

		Particle* particle = GetAssetManager().Get(ps.effect);
		if (!particle || !particle->IsValid()) {
			// Asset missing from cache (never loaded) or Effect ref dead after manager swap.
			log->warn("Invalid particle effect guid='{}' on entity '{}'", ps.effect.GetID(), entity.GetName());
			return;
		}

		if (ps.handle >= 0 && manager->Exists(ps.handle)) {
			manager->StopEffect(ps.handle);
		}

		ps.handle = manager->Play(particle->GetEffect(), pos.x, pos.y, pos.z);
		if (ps.handle < 0) {
			log->warn("Effekseer Play failed for '{}' (guid='{}')", entity.GetName(), ps.effect.GetID());
		}
	}

} // namespace Engine
