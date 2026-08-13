#include "ParticleManager.h"

#include "core/EngineData.h"
#include "components/impl/TransformComponent.h"
#include "components/impl/ParticleSystemComponent.h"

#ifdef AddJob
#undef AddJob
#endif
#include <Camera.h>

#include "scripting/ScriptManager.h"
#include "assets/AssetManager.h"
#include "utils/Utils.h"

#include <tracy/Tracy.hpp>

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

namespace Engine {

	class ParticleManager::DebugTextureLoader : public Effekseer::TextureLoader {
	  public:
		explicit DebugTextureLoader(const EffekseerRendererGL::RendererRef& renderer)
		    : m_renderer(renderer), m_internalLoader(renderer->CreateTextureLoader())
		{
		}

		Effekseer::TextureRef Load(const EFK_CHAR* path, Effekseer::TextureType type) override
		{
			return m_internalLoader->Load(path, type);
		}

		void Unload(Effekseer::TextureRef data) override { m_internalLoader->Unload(data); }

	  private:
		EffekseerRendererGL::RendererRef m_renderer;
		Effekseer::TextureLoaderRef      m_internalLoader;
	};

	void ParticleManager::setLuaBindings()
	{
		GetScriptManager().lua.new_usertype<ParticleManager>("ParticleManager", "playEffect", &ParticleManager::PlayEffect);
		GetScriptManager().lua.set_function("getParticleManager", []() -> ParticleManager& { return Engine::GetParticleManager(); });
	}

	void ParticleManager::onInit()
	{
		ZoneScopedN("Initialize ParticleManager");

		// squareMaxCount sizes Effekseer's sprite GPU buffers.
		m_renderer = EffekseerRendererGL::Renderer::Create(kSquareMax, EffekseerRendererGL::OpenGLDeviceType::OpenGL3);
		if (!m_renderer) {
			log->critical("Failed to create Effekseer renderer");
			return;
		}

		ResetInternalManager();
	}

	void ParticleManager::onGameStart()
	{
		ZoneScopedN("ParticleManager onGameStart");
		auto view = GetCurrentSceneRegistry().view<Components::Transform, Components::ParticleSystem>();
		for (auto [entity, transform, particleSystem] : view.each()) {
			if (!particleSystem.autoPlay || !particleSystem.effect.IsValid()) {
				continue;
			}
			Particle* particle = GetAssetManager().Get(particleSystem.effect);
			if (!particle || !particle->IsValid()) {
				log->error("Particle effect is invalid (guid='{}')", particleSystem.effect.GetID());
				continue;
			}

			const auto pos        = transform.GetWorldPosition();
			particleSystem.handle = m_manager->Play(particle->GetEffect(), pos.x, pos.y, pos.z);
			particleSystem.lastPos  = pos;
			particleSystem.posValid = true;
		}

		// Force Preupdate/Flip so first-frame Draw sees the new instances (Play defers setup to Flip).
		if (m_manager) {
			m_manager->Update(0.0f);
		}
	}

	bool ParticleManager::HasActiveEffects() const
	{
		if (!m_manager) {
			return false;
		}
		// Do NOT use GetTotalInstanceCount() alone — it can be 0 after Play until Preupdate.
		// Handles live in DrawSets immediately after Play (Exists == true).
		auto view = GetCurrentSceneRegistry().view<Components::ParticleSystem>();
		for (auto entity : view) {
			const auto& ps = view.get<Components::ParticleSystem>(entity);
			if (ps.handle >= 0 && m_manager->Exists(ps.handle)) {
				return true;
			}
		}
		return false;
	}

	void ParticleManager::SyncEffectTransforms()
	{
		ZoneScopedN("Particle Sync Transforms");
		if (!m_manager) {
			return;
		}

		auto view = GetCurrentSceneRegistry().view<Components::Transform, Components::ParticleSystem>();
		for (auto [entity, transform, ps] : view.each()) {
			if (ps.handle >= 0 && !m_manager->Exists(ps.handle)) {
				ps.handle   = -1;
				ps.posValid = false;
				continue;
			}

			if (ps.looping && ps.handle < 0 && ps.effect.IsValid()) {
				Particle* particle = GetAssetManager().Get(ps.effect);
				if (particle && particle->IsValid()) {
					const auto pos = transform.GetWorldPosition();
					ps.handle      = m_manager->Play(particle->GetEffect(), pos.x, pos.y, pos.z);
					ps.lastPos     = pos;
					ps.posValid    = true;
				}
				continue;
			}

			if (ps.handle < 0) {
				continue;
			}

			const glm::vec3 pos = transform.GetWorldPosition();
			if (!ps.posValid || glm::any(glm::greaterThan(glm::abs(pos - ps.lastPos), glm::vec3(1e-4f)))) {
				m_manager->SetLocation(ps.handle, pos.x, pos.y, pos.z);
				ps.lastPos  = pos;
				ps.posValid = true;
			}
		}
	}

	void ParticleManager::onUpdate(float dt)
	{
		ZoneScopedN("ParticleManager Update");
		if (!IsSimulating() || !m_manager) {
			return;
		}

		SyncEffectTransforms();

		if (!HasActiveEffects()) {
			return;
		}

		// Effekseer units: 1.0 = 1/60s. Clamp to avoid multi-step death spiral at low FPS.
		float deltaFrame = std::clamp(dt * 60.0f, 0.0f, 2.0f);

		{
			ZoneScopedN("Effekseer Manager::Update");
			m_manager->Update(deltaFrame);
		}
	}

	void ParticleManager::onShutdown()
	{
		m_manager.Reset();
		m_renderer.Reset();
	}

	void ParticleManager::RestoreGLStateAfterParticles()
	{
		glUseProgram(0);
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);

		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
	}

	void ParticleManager::Render()
	{
		ZoneScopedN("ParticleManager Render");
		if (!m_renderer || !m_manager) {
			return;
		}

		if (!HasActiveEffects()) {
			return;
		}

		// Match prior working state setup (depth from bloom combine via gl_FragDepth).
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glDepthMask(GL_FALSE);
		glDisable(GL_CULL_FACE);

		m_renderer->SetProjectionMatrix(ConvertGLMToEffekseerMatrix(GetCamera().GetProjectionMatrix()));
		m_renderer->SetCameraMatrix(ConvertGLMToEffekseerMatrix(GetCamera().GetViewMatrix()));

		{
			ZoneScopedN("Effekseer Draw");
			m_renderer->BeginRendering();
			m_manager->Draw();
			m_renderer->EndRendering();
		}

		glDepthMask(GL_TRUE);
		RestoreGLStateAfterParticles();
	}

	void ParticleManager::ResetInternalManager()
	{
		ZoneScopedN("ResetInternalManager");

		// Single-threaded manager (autoFlip). Worker threads were optional and can race Draw.
		m_manager = Effekseer::Manager::Create(kMaxInstances, true);
		if (!m_manager) {
			log->error("Effekseer Manager not initialized!");
			return;
		}
		log->info("Effekseer manager: maxInstances={}, squareMax={}", kMaxInstances, kSquareMax);

		m_manager->SetSpriteRenderer(m_renderer->CreateSpriteRenderer());
		m_manager->SetRibbonRenderer(m_renderer->CreateRibbonRenderer());
		m_manager->SetRingRenderer(m_renderer->CreateRingRenderer());
		m_manager->SetTrackRenderer(m_renderer->CreateTrackRenderer());
		m_manager->SetModelRenderer(m_renderer->CreateModelRenderer());

		auto textureLoader = Effekseer::MakeRefPtr<DebugTextureLoader>(m_renderer);
		m_manager->SetTextureLoader(textureLoader);

		if (GetCurrentScene()) {
			auto view = GetCurrentSceneRegistry().view<Components::ParticleSystem>();
			for (auto [entity, ps] : view.each()) {
				ps.handle   = -1;
				ps.posValid = false;
			}
		}

		// Effects are bound to a Manager — reload all Particle assets onto the new one.
		auto& storage  = GetAssetManager().GetStorage<Particle>();
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
			log->warn("Invalid particle effect guid='{}' on entity '{}'", ps.effect.GetID(), entity.GetName());
			return;
		}

		if (ps.handle >= 0 && manager->Exists(ps.handle)) {
			manager->StopEffect(ps.handle);
		}

		ps.handle   = manager->Play(particle->GetEffect(), pos.x, pos.y, pos.z);
		ps.lastPos  = pos;
		ps.posValid = true;
		if (ps.handle < 0) {
			log->warn("Effekseer Play failed for '{}' (guid='{}')", entity.GetName(), ps.effect.GetID());
			return;
		}

		// Make the new effect drawable this frame (Play defers instance setup to Flip/Preupdate).
		manager->Update(0.0f);
	}

} // namespace Engine
