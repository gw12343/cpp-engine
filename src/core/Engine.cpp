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
// #include "serialization/SerializationManager.h"


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
// #include "components/impl/ModelRendererComponent.h"
// #include "components/impl/AudioSourceComponent.h"
// #include "components/impl/ShadowCasterComponent.h"
// #include "components/impl/LuaScriptComponent.h"
// #include "components/impl/SkeletonComponent.h"
// #include "components/impl/AnimationComponent.h"
// #include "components/impl/AnimationPoseComponent.h"
// #include "components/impl/RigidBodyComponent.h"
// #include "components/impl/TerrainRendererComponent.h"
//
// #include "terrain/TerrainManager.h"
#include "assets/impl/SceneLoader.h"

namespace Engine {
	ModuleManager manager;


	GEngine::GEngine(int width, int height, const char* title) : m_deltaTime(0.0f), m_lastFrame(0.0f)
	{
		// Initialize asset loaders
		Get().assetManager = std::make_shared<AssetManager>();
		GetAssetManager().RegisterLoader<Texture>(std::make_unique<TextureLoader>());
		GetAssetManager().RegisterLoader<Rendering::Model>(std::make_unique<Rendering::ModelLoader>());
		GetAssetManager().RegisterLoader<Terrain::TerrainTile>(std::make_unique<TerrainLoader>());
		GetAssetManager().RegisterLoader<Audio::SoundBuffer>(std::make_unique<SoundLoader>());
		GetAssetManager().RegisterLoader<Scene>(std::make_unique<SceneLoader>());

		// Initialize Scene
		// Get().registry = std::make_shared<entt::registry>();

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
		manager.RegisterExternal(Get().ui);
		manager.RegisterExternal(Get().animation);
		manager.RegisterExternal(Get().particle);
		manager.RegisterExternal(Get().terrain);
		manager.RegisterExternal(Get().renderer);
		manager.RegisterExternal(Get().script);
		manager.RegisterExternal(Get().scene);
	}


	bool GEngine::Initialize()
	{
		SPDLOG_INFO("Starting Engine");

		Components::RegisterAllComponentBindings();
		manager.InitAllLuaBindings();
		manager.InitAll();
		CreateInitialEntities();

		return true;
	}


	void GEngine::CreateInitialEntities()
	{
		AssetHandle<Rendering::Model> treeModel = GetAssetManager().Load<Rendering::Model>("resources/models/TwistedTree_1.obj");


		GetAssetManager().Load<Rendering::Model>("resources/models/Spruce2.fbx");
		GetAssetManager().Load<Rendering::Model>("resources/models/cube.obj");
		GetAssetManager().Load<Rendering::Model>("resources/models/sphere.obj");
		AssetHandle<Rendering::Model>   cubeModel = AssetHandle<Rendering::Model>("eb048fc8e5c5aa8d185a48312731d0f1");
		AssetHandle<Audio::SoundBuffer> snd       = GetAssetManager().Load<Audio::SoundBuffer>("resources/sounds/bird_chirp.wav");


		//		Entity entity = Entity::Create("TestEntity");
		//		entity.AddComponent<Components::ModelRenderer>(treeModel);
		//		entity.AddComponent<Components::Transform>(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
		//		entity.AddComponent<Components::ParticleSystem>("resources/particles/testleaf.efk");


		//		Entity entity2 = Entity::Create("TestEntity2");
		//		entity2.AddComponent<Components::ModelRenderer>(treeModel);
		//		entity2.AddComponent<Components::Transform>(glm::vec3(2.0f, 0.0f, -5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
		//
		//
		//		entity2.AddComponent<Components::AudioSource>(snd, true, 0.1f, 1.0f, true, 5.0f, 50.0f, 1.0f);
		//		entity2.AddComponent<Components::ShadowCaster>();
		//		entity2.AddComponent<Components::LuaScript>("scripts/test.lua");
		//
		//
		//		Entity animatedEntity = Entity::Create("AnimatedEntity");
		//		animatedEntity.AddComponent<Components::Transform>(glm::vec3(0.0f, 3.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
		//		animatedEntity.AddComponent<Components::SkeletonComponent>("resources/models/ruby_skeleton.ozz");
		//		animatedEntity.AddComponent<Components::AnimationComponent>("resources/models/ruby_animation.ozz");
		//		animatedEntity.AddComponent<Components::AnimationPoseComponent>();
		//		animatedEntity.AddComponent<Components::AnimationWorkerComponent>();
		//		animatedEntity.AddComponent<Components::SkinnedMeshComponent>("resources/models/ruby_mesh.ozz");


		// Terrain::TerrainTile tile = Terrain::LoadTerrainTile("resources/terrain/terrain.bin");
		// std::cout << "Loaded tile: " << tile.name << "\n";
		// std::cout << "Heights: " << tile.heightmap.size() << "\n";
		// std::cout << "Trees: " << tile.trees.size() << "\n";
		//


		//		Entity terrainWrapper = Entity::Create("TerrainWrapper");
		//		terrainWrapper.AddComponent<Components::Transform>(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
		//
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
		// std::vector<Entity> ets = GetSerializationManager().LoadScene(GetRegistry(), "debug/savescene.json");

		GetSceneManager().SetActiveScene(GetAssetManager().Load<Scene>("scenes/scene1.json"));

		while (!GetWindow().ShouldClose()) {
			auto currentFrame = static_cast<float>(glfwGetTime());
			m_deltaTime       = currentFrame - m_lastFrame;
			m_lastFrame       = currentFrame;

			manager.UpdateAll(m_deltaTime);
		}
		// GetSerializationManager().SaveScene(GetRegistry(), "debug/savescene.json");
	}


	void GEngine::Shutdown()
	{
		manager.ShutdownAll();

		Components::AnimationWorkerComponent::CleanAnimationContexts();
		Components::SkinnedMeshComponent::CleanSkinnedModels();
		Texture::CleanAllTextures();
		Rendering::Mesh::CleanAllMeshes();
		SPDLOG_INFO("Engine shutdown complete");
	}
} // namespace Engine
#include "assets/AssetManager.inl"