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


#include "Window.h"
#include "rendering/Renderer.h"
#include "physics/PhysicsManager.h"

#include "animation/AnimationManager.h"
#include "rendering/particles/ParticleManager.h"
#include "rendering/ui/UIManager.h"
#include "terrain/TerrainManager.h"
#include "components/impl/AnimationComponent.h"
#include "components/impl/LuaScriptComponent.h"
#include "components/impl/ModelRendererComponent.h"
#include "components/impl/AudioSourceComponent.h"
#include "components/impl/ShadowCasterComponent.h"
#include "components/impl/SkeletonComponent.h"
#include "components/impl/AnimationPoseComponent.h"
#include "components/impl/AnimationWorkerComponent.h"
#include "components/impl/SkinnedMeshComponent.h"
#include "assets/AssetManager.h"
#include "assets/impl/TextureLoader.h"
#include "assets/impl/TerrainLoader.h"
#include "components/impl/RigidBodyComponent.h"
#include "animation/TerrainRendererComponent.h"

namespace Engine {
	ModuleManager manager;


	GEngine::GEngine(int width, int height, const char* title) : m_deltaTime(0.0f), m_lastFrame(0.0f)
	{
		// Initialize asset loaders
		Get().assetManager = std::make_shared<AssetManager>();
		GetAssetManager().RegisterLoader<Texture>(std::make_unique<TextureLoader>());
		GetAssetManager().RegisterLoader<Rendering::Model>(std::make_unique<Rendering::ModelLoader>());
		GetAssetManager().RegisterLoader<Terrain::TerrainTile>(std::make_unique<TerrainLoader>());

		// Initialize Scene
		Get().registry = std::make_shared<entt::registry>();

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


		// cube          = Rendering::ModelLoader::LoadModel("resources/models/cube.obj");
		//		Entity entity = Entity::Create("TestEntity");
		//		entity.AddComponent<Components::ModelRenderer>(treeModel);
		//		entity.AddComponent<Components::Transform>(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
		//		entity.AddComponent<Components::ParticleSystem>("resources/particles/testleaf.efk");


		Entity entity2 = Entity::Create("TestEntity2");
		entity2.AddComponent<Components::ModelRenderer>(treeModel);
		entity2.AddComponent<Components::Transform>(glm::vec3(2.0f, 0.0f, -5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
		entity2.AddComponent<Components::AudioSource>("birds", true, 0.1f, 1.0f, true, 5.0f, 50.0f, 1.0f);
		entity2.AddComponent<Components::ShadowCaster>();
		entity2.AddComponent<Components::LuaScript>("scripts/test.lua");


		Entity animatedEntity = Entity::Create("AnimatedEntity");
		animatedEntity.AddComponent<Components::Transform>(glm::vec3(0.0f, 3.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
		animatedEntity.AddComponent<Components::SkeletonComponent>("resources/models/ruby_skeleton.ozz");
		animatedEntity.AddComponent<Components::AnimationComponent>("resources/models/ruby_animation.ozz");
		animatedEntity.AddComponent<Components::AnimationPoseComponent>();
		animatedEntity.AddComponent<Components::AnimationWorkerComponent>();
		animatedEntity.AddComponent<Components::SkinnedMeshComponent>("resources/models/ruby_mesh.ozz");


		//		Terrain::TerrainTile tile = Terrain::LoadTerrainTile("resources/terrain/terrain.bin");
		//		std::cout << "Loaded tile: " << tile.name << "\n";
		//		std::cout << "Heights: " << tile.heightmap.size() << "\n";
		//		std::cout << "Trees: " << tile.trees.size() << "\n";
		//


		Entity terrainWrapper = Entity::Create("TerrainWrapper");
		terrainWrapper.AddComponent<Components::Transform>(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));


		auto&                             body_interface = GetPhysics().GetPhysicsSystem()->GetBodyInterface();
		AssetHandle<Terrain::TerrainTile> terrain        = GetAssetManager().Load<Terrain::TerrainTile>("resources/terrain/TerrainA.bin");

		auto  tile         = GetAssetManager().Get(terrain); // GetTerrainManager().GetTerrains()[0];
		Body* terrain_body = body_interface.CreateBody(BodyCreationSettings(new MeshShapeSettings(tile->physicsMesh), RVec3(0.0_r, 0.0_r, 0.0_r), Quat::sIdentity(), EMotionType::Static, Layers::NON_MOVING));
		spdlog::info("id {}", terrain_body->GetID().GetIndex());
		body_interface.AddBody(terrain_body->GetID(), EActivation::DontActivate);

		terrainWrapper.AddComponent<Components::RigidBodyComponent>(terrain_body->GetID());
		terrainWrapper.AddComponent<Components::TerrainRenderer>(terrain);
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
		manager.ShutdownAll();

		Components::AnimationWorkerComponent::CleanAnimationContexts();
		Components::SkinnedMeshComponent::CleanSkinnedModels();
		Texture::CleanAllTextures();
		Rendering::Mesh::CleanAllMeshes();
		SPDLOG_INFO("Engine shutdown complete");
	}
} // namespace Engine
