#include "ParticleManager.h"

// #include "components/Components.h"
#include "utils/Utils.h"

// #include <components/Components.h>
#include <spdlog/spdlog.h>
namespace Engine {

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

	ParticleManager::ParticleManager() = default;

	ParticleManager::~ParticleManager()
	{
		Shutdown();
	}

	bool ParticleManager::Initialize(int maxInstances)
	{
		m_renderer = EffekseerRendererGL::Renderer::Create(maxInstances, EffekseerRendererGL::OpenGLDeviceType::OpenGL3);
		if (!m_renderer) {
			spdlog::critical("Failed to create Effekseer renderer");
			return false;
		}

		m_manager = Effekseer::Manager::Create(maxInstances);
		m_manager->SetSpriteRenderer(m_renderer->CreateSpriteRenderer());
		m_manager->SetRibbonRenderer(m_renderer->CreateRibbonRenderer());
		m_manager->SetRingRenderer(m_renderer->CreateRingRenderer());
		m_manager->SetTrackRenderer(m_renderer->CreateTrackRenderer());
		m_manager->SetModelRenderer(m_renderer->CreateModelRenderer());

		auto textureLoader = Effekseer::MakeRefPtr<DebugTextureLoader>(m_renderer);
		m_manager->SetTextureLoader(textureLoader);

		//
		////		auto effect = Effekseer::Effect::Create(m_manager, u"/home/gabe/CLionProjects/cpp-engine/resources/particles/fireworks.efk");
		//		// auto effect = Effekseer::Effect::Create(m_particleManager, u"/home/gabe/CLionProjects/cpp-engine/build/_deps/effekseer-src/Examples/Resources/Laser01.efkefc");
		//		if (!effect) {
		//			SPDLOG_CRITICAL("Failed to load resource");
		//		}
		//
		//		int handle = m_manager->Play(effect, 0.0f, 0.0f, 0.0f);
		//

		if (!m_manager) {
			spdlog::error("Effekseer Manager not initialized!");
			// handle error, maybe return early
		}
		else {
			spdlog::info("Effekseer Manager initialized!");
		}

		return true;
	}

	void ParticleManager::Update(entt::registry& registry, float deltaTime)
	{
		if (m_manager) {
			m_manager->Update(deltaTime * 60.0);
		}
	}

	void ParticleManager::Render(Window& window, Camera& camera)
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
			::Effekseer::Matrix44 projMat = ConvertGLMToEffekseerMatrix(camera.GetProjectionMatrix(window.GetAspectRatio()));
			::Effekseer::Matrix44 viewMat = ConvertGLMToEffekseerMatrix(camera.GetViewMatrix());


			m_renderer->SetProjectionMatrix(projMat);
			m_renderer->SetCameraMatrix(viewMat);

			m_renderer->BeginRendering();
			m_manager->Draw();
			m_renderer->EndRendering();
		}
		glDepthMask(GL_TRUE);
	}

	void ParticleManager::Shutdown()
	{
		m_manager.Reset();
		m_renderer.Reset();
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
