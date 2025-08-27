#include "Engine.h"

#include "components/Components.h"
#include "assets/impl/ModelLoader.h"
#include "Jolt/Physics/Collision/Shape/MeshShape.h"
#include "core/module/ModuleManager.h"
#include <memory>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "EngineData.h"
#include "Input.h"
#include "scripting/ScriptManager.h"

#include "SceneManager.h"
#include <filesystem>
#include <string>

#include "Window.h"
#include "rendering/Renderer.h"
#include "physics/PhysicsManager.h"

#include "animation/AnimationManager.h"
#include "rendering/particles/ParticleManager.h"
#include "rendering/ui/UIManager.h"
#include "terrain/TerrainManager.h"
#include "components/impl/AnimationWorkerComponent.h"
#include "components/impl/SkinnedMeshComponent.h"
#include "assets/AssetManager.h"
#include "assets/impl/TextureLoader.h"
#include "assets/impl/TerrainLoader.h"
#include "assets/impl/SoundLoader.h"
#include "assets/impl/JSONSceneLoader.h"
#include "rendering/particles/Particle.h"
#include "assets/impl/ParticleLoader.h"
#include "assets/impl/MaterialLoader.h"
#include "assets/impl/BinarySceneLoader.h"


namespace fs = std::filesystem;

namespace Engine {
	ModuleManager manager;


	GEngine::GEngine(int width, int height, const char* title) : m_deltaTime(0.0f), m_lastFrame(0.0f)
	{
		SetState(EDITOR);
		Get().manager = &manager;
		// Initialize asset loaders
		Get().assetManager = std::make_shared<AssetManager>();
		GetAssetManager().RegisterLoader<Texture>(std::make_unique<TextureLoader>());
		GetAssetManager().RegisterLoader<Rendering::Model>(std::make_unique<Rendering::ModelLoader>());
		GetAssetManager().RegisterLoader<Terrain::TerrainTile>(std::make_unique<TerrainLoader>());
		GetAssetManager().RegisterLoader<Audio::SoundBuffer>(std::make_unique<SoundLoader>());
		GetAssetManager().RegisterLoader<Scene>(std::make_unique<SCENE_LOADER>());
		GetAssetManager().RegisterLoader<Particle>(std::make_unique<ParticleLoader>());
		GetAssetManager().RegisterLoader<Material>(std::make_unique<MaterialLoader>());

		// Initialize Modules
		Get().window    = std::make_shared<Window>(width, height, title);
		Get().input     = std::make_shared<Input>();
		Get().camera    = std::make_shared<Camera>(glm::vec3(0.0f, 3.0f, 6.0f), glm::vec3(0, 1, 0), -90.0f, -30.0f);
		Get().renderer  = std::make_shared<Renderer>();
		Get().physics   = std::make_shared<PhysicsManager>();
		Get().sound     = std::make_shared<Audio::SoundManager>();
		Get().animation = std::make_shared<AnimationManager>();
		Get().particle  = std::make_shared<ParticleManager>();
		Get().ui        = std::make_shared<UI::UIManager>();
		Get().terrain   = std::make_shared<Terrain::TerrainManager>();
		Get().script    = std::make_shared<ScriptManager>();
		Get().scene     = std::make_shared<SceneManager>();

		// Register Modules to handle lifecycle
		manager.RegisterExternal(Get().window);
		manager.RegisterExternal(Get().input);
		manager.RegisterExternal(Get().camera);
		manager.RegisterExternal(Get().physics);
		manager.RegisterExternal(Get().sound);
#ifndef GAME_BUILD
		manager.RegisterExternal(Get().ui);
#endif
		manager.RegisterExternal(Get().animation);
		manager.RegisterExternal(Get().particle);
		manager.RegisterExternal(Get().terrain);
		manager.RegisterExternal(Get().renderer);
		manager.RegisterExternal(Get().script);
		manager.RegisterExternal(Get().scene);
	}

	bool GEngine::Initialize()
	{
		GetDefaultLogger()->info("Starting Engine");
		Components::RegisterAllComponentBindings();
		manager.InitAllLuaBindings();
		manager.InitAll();

		AssetHandle<Particle> testParticle = GetAssetManager().Load<Particle>("resources/particles/testleaf.efk");
		GetSceneManager().SetActiveScene(GetAssetManager().Load<Scene>(SCENE1));
		CreateInitialEntities();



#ifdef GAME_BUILD
		SetState(PLAYING);
		Get().manager->StartGame();
#endif

		return true;
	}


	void GEngine::CreateInitialEntities()
	{
		// TODO store assets to be loaded at the start in scene json

		// Load all textures
		std::string folder = "resources/textures";
		for (const auto& entry : fs::directory_iterator(folder)) {
			if (entry.is_regular_file() && entry.path().extension() == ".png") {
				std::string path = entry.path().string();
				GetAssetManager().Load<Texture>(path);
			}
		}
		// Load all materials
		folder = "resources/materials";
		for (const auto& entry : fs::directory_iterator(folder)) {
			if (entry.is_regular_file() && entry.path().extension() == ".material") {
				std::string path = entry.path().string();
				GetAssetManager().Load<Material>(path);
			}
		}
		// Load all models
		folder = "resources/models";
		for (const auto& entry : fs::directory_iterator(folder)) {
			if (entry.is_regular_file() && entry.path().extension() == ".obj") {
				std::string path = entry.path().string();
				GetAssetManager().Load<Rendering::Model>(path);
			}
		}

		// Load all sounds
		folder = "resources/sounds";
		for (const auto& entry : fs::directory_iterator(folder)) {
			if (entry.is_regular_file() && entry.path().extension() == ".wav") {
				std::string path = entry.path().string();
				GetAssetManager().Load<Audio::SoundBuffer>(path);
			}
		}

		// TODO terrain instanced detail rendereing
		// TODO terrain mesh shape?? maybe component

		//  AssetHandle<Terrain::TerrainTile> terr        = GetAssetManager().Load<Terrain::TerrainTile>("resources/terrain/terrain1.bin");

		//		Entity terrainWrapper = Entity::Create("TerrainWrapper");
		//		terrainWrapper.AddComponent<Components::Transform>(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
		//
		//		auto&                             body_interface = GetPhysics().GetPhysicsSystem()->GetBodyInterface();
		//		AssetHandle<Terrain::TerrainTile> terrain        = GetAssetManager().Load<Terrain::TerrainTile>("resources/terrain/TerrainA.bin");
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
			auto currentFrame = static_cast<float>(glfwGetTime());
			m_deltaTime       = currentFrame - m_lastFrame;
			m_lastFrame       = currentFrame;

			manager.UpdateAll(m_deltaTime);
		}
	}


	void GEngine::Shutdown()
	{
        #ifndef GAME_BUILD
		 BinarySceneLoader::SerializeScene(GetSceneManager().GetActiveScene(), "scenes/scene1.bin");
        #endif
		manager.ShutdownAll();

		Components::AnimationWorkerComponent::CleanAnimationContexts();
		Components::SkinnedMeshComponent::CleanSkinnedModels();
		Texture::CleanAllTextures();
		Rendering::Mesh::CleanAllMeshes();
		GetDefaultLogger()->info("Engine shutdown complete");
	}
} // namespace Engine
#include "assets/AssetManager.inl"