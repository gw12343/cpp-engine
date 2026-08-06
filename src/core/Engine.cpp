#include "Engine.h"

#include "components/Components.h"
#include "assets/impl/ModelLoader.h"
#include "Jolt/Physics/Collision/Shape/MeshShape.h"
#include "core/module/ModuleManager.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include "EngineData.h"
#include "Input.h"
#include "scripting/ScriptManager.h"

#include "SceneManager.h"


#include "Window.h"
#include "rendering/Renderer.h"
#include "physics/PhysicsManager.h"

#include "animation/AnimationManager.h"
#include "rendering/particles/ParticleManager.h"
#include "rendering/ui/UIManager.h"
#include "rendering/ui/GameUIManager.h"
#include "terrain/TerrainManager.h"
#include "components/impl/SkinnedMeshComponent.h"

#include "assets/impl/TextureLoader.h"
#include "assets/impl/TerrainLoader.h"
#include "assets/impl/SoundLoader.h"
#include "assets/impl/JSONSceneLoader.h"
#include "rendering/particles/Particle.h"
#include "assets/impl/ParticleLoader.h"
#include "assets/impl/MaterialLoader.h"
#include "assets/impl/BinarySceneLoader.h"
#include "assets/impl/AnimationLoader.h"
#include "components/impl/AnimationComponent.h"
#include "components/impl/EntityMetadataComponent.h"
#include "core/Entity.h"
#include "core/Scene.h"
#include "utils/Logger.h"
#include "assets/AssetWatcher.h"

#if defined(__clang__) || defined(__GNUC__)
#define TracyFunction __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
#define TracyFunction __FUNCSIG__
#endif

#ifndef GAME_BUILD
#define TRACY_ENABLE
#endif


#include "TracyClient.cpp"


namespace fs = std::filesystem;

namespace Engine {

	GEngine::GEngine(int width, int height, const char* title) : m_deltaTime(0.0f), m_lastFrame(0.0f)
	{
		ZoneScopedN("Engine Awake");
		SetState(EDITOR);

		m_moduleManager = std::make_unique<ModuleManager>();
		Get().manager   = m_moduleManager.get();

		Get().renderSettings = new RenderSettings();
		// Initialize asset loaders
		Get().assetManager = std::make_shared<AssetManager>();
		GetAssetManager().RegisterLoader<Texture>(std::make_unique<TextureLoader>());
		GetAssetManager().RegisterLoader<Rendering::Model>(std::make_unique<Rendering::ModelLoader>());
		GetAssetManager().RegisterLoader<Terrain::TerrainTile>(std::make_unique<TerrainLoader>());
		GetAssetManager().RegisterLoader<Audio::SoundBuffer>(std::make_unique<SoundLoader>());
		GetAssetManager().RegisterLoader<Scene>(std::make_unique<SCENE_LOADER>());
		GetAssetManager().RegisterLoader<Particle>(std::make_unique<ParticleLoader>());
		GetAssetManager().RegisterLoader<Material>(std::make_unique<MaterialLoader>());
		GetAssetManager().RegisterLoader<Animation>(std::make_unique<AnimationLoader>());

		// Initialize Modules
		Get().script    = std::make_shared<ScriptManager>();
		Get().window    = std::make_shared<Window>(width, height, title);
		Get().input     = std::make_shared<Input>();
		Get().camera    = std::make_shared<Camera>(glm::vec3(0.0f, 3.0f, 6.0f), glm::vec3(0, 1, 0), -90.0f, -30.0f);
		Get().renderer  = std::make_shared<Renderer>();
		Get().physics   = std::make_shared<PhysicsManager>();
		Get().sound     = std::make_shared<Audio::SoundManager>();
		Get().animation = std::make_shared<AnimationManager>();
		Get().particle  = std::make_shared<ParticleManager>();
		Get().ui        = std::make_shared<UI::UIManager>();
		Get().gameUI    = std::make_shared<GameUIManager>();
		Get().terrain   = std::make_shared<Terrain::TerrainManager>();
		Get().scene     = std::make_shared<SceneManager>();

		// Register Modules to handle lifecycle
		auto& modules = *m_moduleManager;
		modules.RegisterExternal(Get().script); // ScriptManager must run first to clear subscriptions before UI reloads
		modules.RegisterExternal(Get().window);
		modules.RegisterExternal(Get().input);
		modules.RegisterExternal(Get().camera);
		modules.RegisterExternal(Get().physics);
		modules.RegisterExternal(Get().sound);
#ifndef GAME_BUILD
		modules.RegisterExternal(Get().ui);
#endif
		modules.RegisterExternal(Get().gameUI);
		// Animation before renderer so poses are current before PrepareSkinnedMeshes / draws.
		modules.RegisterExternal(Get().animation);
		modules.RegisterExternal(Get().renderer);
		modules.RegisterExternal(Get().particle);
		modules.RegisterExternal(Get().terrain);
		modules.RegisterExternal(Get().scene);

#ifndef GAME_BUILD
		m_assetFileWatcher = std::make_unique<efsw::FileWatcher>();
		m_assetWatcher     = std::make_unique<HotReloadWatcher>();
		m_assetFileWatcher->addWatch("resources", m_assetWatcher.get(), true);
		m_assetFileWatcher->watch();
#endif
	}

	GEngine::~GEngine() = default;

	bool GEngine::Initialize()
	{
		ZoneScopedN("Engine Init");
		GetDefaultLogger()->info("Starting Engine");
		{
			ZoneScopedN("RegisterAllComponentBindings");
			Components::RegisterAllComponentBindings();
		}
		{
			ZoneScopedN("InitAllLuaBindings");
			m_moduleManager->InitAllLuaBindings();
		}
		{
			ZoneScopedN("Init All Modules");
			m_moduleManager->InitAll();
		}
		// ParticleHandle testParticle = GetAssetManager().Load<Particle>("resources/particles/testleaf.efk");
		{
			ZoneScopedN("Load Game Assets");
			LoadGameAssets();
		}
		{
			ZoneScopedN("Load Scene 1");
			GetSceneManager().SetActiveScene(GetAssetManager().Load<Scene>(SCENE1));
		}

#ifdef GAME_BUILD
		SetState(PLAYING);
		Get().manager->StartGame();
#endif

		return true;
	}


	void GEngine::LoadGameAssets()
	{
		using LoaderFn = std::function<void(const std::string&)>;

		std::unordered_map<std::string, LoaderFn> loaders = {
		    {".png", [this](const std::string& p) { GetAssetManager().Load<Texture>(p); }},
		    {".material", [this](const std::string& p) { GetAssetManager().Load<Material>(p); }},
		    {".obj", [this](const std::string& p) { GetAssetManager().Load<Rendering::Model>(p); }},
		    {".wav", [this](const std::string& p) { GetAssetManager().Load<Audio::SoundBuffer>(p); }},
		    {".anim", [this](const std::string& p) { GetAssetManager().Load<Animation>(p); }},
		    {".bin", [this](const std::string& p) { GetAssetManager().Load<Terrain::TerrainTile>(p); }},
		    {".efk", [this](const std::string& p) { GetAssetManager().Load<Particle>(p); }},
		};

		auto loadFromAssetSubfolder = [&](const std::string& assetSubfolder) {
			int loadedCount = 0;

			try {
				// Use current working directory as project root
				fs::path projectRootCanonical = fs::canonical(fs::current_path());

				// Asset folder absolute path under project root
				fs::path assetFolderCanonical = fs::canonical(projectRootCanonical / assetSubfolder);

				for (const auto& entry : fs::recursive_directory_iterator(assetFolderCanonical)) {
					if (!entry.is_regular_file()) continue;

					const fs::path& path = entry.path();

					std::string filename = path.filename().string();
					if (!filename.empty() && filename[0] == '.') continue;

					std::string ext = path.extension().string();

					auto it = loaders.find(ext);
					if (it == loaders.end()) continue;

					fs::path assetCanonical = fs::canonical(path);

					// Make path relative to the project root
					fs::path relativeToProjectRoot = fs::relative(assetCanonical, projectRootCanonical);

					std::string finalPath = relativeToProjectRoot.generic_string();
					GetDefaultLogger()->info("loading asset: {}", finalPath.c_str());

					{
						ZoneScoped;
						char* buff = new char[150];
						sprintf(buff, "Load: %s", finalPath.c_str());


						ZoneName(buff, strlen(buff));


						try {
							it->second(finalPath);
							loadedCount++;
						}
						catch (const std::exception& e) {
							GetDefaultLogger()->warn("Failed to load asset {}: {}", finalPath, e.what());
						}
					}
				}
			}
			catch (const std::exception& e) {
				GetDefaultLogger()->error("Error reading asset folder {}: {}", assetSubfolder, e.what());
			}
		};


		loadFromAssetSubfolder("resources");
		loadFromAssetSubfolder("assets");


		// TODO store assets to be loaded at the start in scene json

		// TODO terrain instanced detail rendereing
		// TODO terrain mesh shape?? maybe component

		//  TerrainHandle terr        = GetAssetManager().Load<Terrain::TerrainTile>("resources/terrain/terrain1.bin");

		//		Entity terrainWrapper = Entity::Create("TerrainWrapper");
		//		terrainWrapper.AddComponent<Components::Transform>(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
		//
		//		auto&                             body_interface = GetPhysics().GetPhysicsSystem()->GetBodyInterface();
		//		TerrainHandle terrain        = GetAssetManager().Load<Terrain::TerrainTile>("resources/terrain/TerrainA.bin");
		//		auto                              tile           = GetAssetManager().Get(terrain);
		//
		//		if (!tile->heightfieldShape) {
		//			spdlog::error("Terrain has no heightfield shape!");
		//		}
		//		else {
		//			Body* terrain_body = body_interface.CreateBody(BodyCreationSettings(tile->heightfieldShape, JPH::RVec3(tile->posX, tile->posY, tile->posZ), JPH::Quat::sIdentity(), JPH::EMotionType::Static, Layers::NON_MOVING));
		//			body_interface.AddBody(terrain_body->GetID(), JPH::EActivation::DontActivate);
		//
		//			terrainWrapper.AddComponent<Components::RigidBodyComponent>(terrain_body->GetID());
		//		}
		//		terrainWrapper.AddComponent<Components::TerrainRenderer>(terrain);
		//		auto tr = GetAssetManager().Get(terrain);
		//		for (auto tree : tr->trees) {
		//			glm::vec3 pos     = {tree.x * tr->sizeX, tree.y * tr->sizeY, tree.z * tr->sizeZ};
		//			Entity    entity2 = Entity::Create("tree");
		//			entity2.AddComponent<Components::ModelRenderer>(cubeModel);
		//			entity2.AddComponent<Components::Transform>(pos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.01f, 0.01f, 0.01f));
		//			auto& rb = entity2.AddComponent<Components::RigidBodyComponent>();
		//			rb.SetKinematic(true);
		//			rb.SetCylinderShape(CylinderShapeSettings(2.5, 0.25));
		//		}
	}

	void GEngine::Run()
	{
		while (!GetWindow().ShouldClose()) {
        TracyCFrameMark
			//FrameMarkStart("main");
			auto currentFrame = static_cast<float>(glfwGetTime());
			m_deltaTime       = currentFrame - m_lastFrame;
			m_lastFrame       = currentFrame;

			GetAssetManager().Update();
			m_moduleManager->UpdateAll(m_deltaTime);
			//FrameMarkEnd("main");
		}
	}


	void GEngine::Shutdown()
	{
		Components::AnimationComponent::CleanAnimationContexts();
		Components::SkinnedMeshComponent::CleanSkinnedModels();

		if (Get().assetManager && Get().scene) {
			Scene* scene = GetCurrentScene();
			if (scene) {
				std::vector<Entity> roots;
				roots.reserve(scene->m_entityList.size());
				for (Entity& e : scene->m_entityList) {
					if (!e.IsValid()) continue;
					if (!e.HasComponent<Components::EntityMetadata>()) continue;
					if (!e.GetComponent<Components::EntityMetadata>().parentEntity.IsValid()) {
						roots.push_back(e);
					}
				}
				for (Entity& e : roots) {
					if (e.IsValid()) {
						e.Destroy();
					}
				}
				scene->m_entityList.clear();
				scene->m_entityMap.clear();
			}
		}

		if (Get().assetManager) {
			GetAssetManager().ClearAll();
		}

		if (m_moduleManager) {
			m_moduleManager->ShutdownAll();
		}

		auto& data = Get();
		data.gameUI.reset();
		data.ui.reset();
		data.input.reset();
		data.camera.reset();
		data.physics.reset();
		data.script.reset();
		data.terrain.reset();
		data.particle.reset();
		data.animation.reset();
		data.sound.reset();
		data.renderer.reset();
		data.window.reset();
		data.scene.reset();
		data.assetManager.reset();
		delete data.renderSettings;
		data.renderSettings = nullptr;

		data.manager = nullptr;
		if (m_moduleManager) {
			m_moduleManager->Clear();
			m_moduleManager.reset();
		}

#ifndef GAME_BUILD
		m_assetFileWatcher.reset();
		m_assetWatcher.reset();
#endif

		Logger::Shutdown();
	}
} // namespace Engine
#include "assets/AssetManager.inl"