#include "ParticleManager.h"

// #include "components/Components.h"
#include "utils/Utils.h"
#include "components/Components.h"
#include "core/EngineData.h"

// #include <components/Components.h>
#include <spdlog/spdlog.h>
namespace Engine {
	static const int MAX_INSTANCES = 8000;

	class ParticleManager::DebugTextureLoader : public Effekseer::TextureLoader {
	  public:
		DebugTextureLoader(EffekseerRendererGL::RendererRef renderer) : m_renderer(renderer), m_internalLoader(renderer->CreateTextureLoader()) {}

		Effekseer::TextureRef Load(const EFK_CHAR* path, Effekseer::TextureType type) override
		{
			std::u16string u16Path(path);
			std::string    pathStr(u16Path.begin(), u16Path.end());
			spdlog::debug("Effekseer trying to load texture: {}", pathStr);

			auto tex = m_internalLoader->Load(path, type);
			if (!tex) {
				spdlog::error("Failed to load texture: {}", pathStr);
			}
			return tex;
		}

		void Unload(Effekseer::TextureRef data) override { m_internalLoader->Unload(data); }

	  private:
		EffekseerRendererGL::RendererRef m_renderer;
		Effekseer::TextureLoaderRef      m_internalLoader;
	};


	void ParticleManager::onInit()
	{
		m_renderer = EffekseerRendererGL::Renderer::Create(MAX_INSTANCES, EffekseerRendererGL::OpenGLDeviceType::OpenGL3);
		if (!m_renderer) {
			spdlog::critical("Failed to create Effekseer renderer");
			return;
		}

		m_manager = Effekseer::Manager::Create(MAX_INSTANCES);
		m_manager->SetSpriteRenderer(m_renderer->CreateSpriteRenderer());
		m_manager->SetRibbonRenderer(m_renderer->CreateRibbonRenderer());
		m_manager->SetRingRenderer(m_renderer->CreateRingRenderer());
		m_manager->SetTrackRenderer(m_renderer->CreateTrackRenderer());
		m_manager->SetModelRenderer(m_renderer->CreateModelRenderer());

		auto textureLoader = Effekseer::MakeRefPtr<DebugTextureLoader>(m_renderer);
		m_manager->SetTextureLoader(textureLoader);

		if (!m_manager) {
			spdlog::error("Effekseer Manager not initialized!");
			return;
		}
		else {
			spdlog::info("Effekseer Manager initialized!");
		}
	}


	void ParticleManager::onUpdate(float dt)
	{
		if (m_manager) {
			m_manager->Update(dt * 60.0);
		}

		// Update particles systems' locations
		auto view = GetRegistry().view<Components::Transform, Components::ParticleSystem>();
		for (auto [entity, transform, particleSystem] : view.each()) {
			m_manager->SetLocation(particleSystem.handle, transform.position.x, transform.position.y, transform.position.z); // particleSystem.UpdateTransform(transform);
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
		glEnable(GL_CULL_FACE);
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


	Effekseer::Handle ParticleManager::PlayEffect(const std::u16string& path, float x, float y, float z)
	{
		Effekseer::RefPtr<Effekseer::Effect> effect = Effekseer::Effect::Create(m_manager, path.c_str());
		if (!effect) {
			spdlog::critical("Failed to load particle effect: {}", std::string(path.begin(), path.end()));
			return -1;
		}

		return m_manager->Play(effect, x, y, z);
	}


} // namespace Engine
