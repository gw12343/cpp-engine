#include "Engine.h"

#include "Entity.h"
#include "Input.h"
#include "components/Components.h"
#include "physics/PhysicsManager.h"
#include "terrain/TerrainLoader.h"
#include "terrain/TerrainManager.h"
#include "utils/ModelLoader.h"
#include "Jolt/Physics/Collision/Shape/MeshShape.h"
#include "rendering/Framebuffer.h"
#include "rendering/ui/UIManager.h"
#include <imgui.h>
#include <memory>
#include <spdlog/sinks/stdout_color_sinks.h>

using namespace JPH;
using namespace JPH::literals;


namespace Engine {


	GEngine::GEngine(int width, int height, const char* title)
	    : m_window(width, height, title), m_camera(glm::vec3(0.0f, 3.0f, 6.0f), glm::vec3(0, 1, 0), -90.0f, -30.0f), m_soundManager(std::make_shared<Audio::SoundManager>()), m_animationManager(std::make_unique<AnimationManager>(this)),
	      m_deltaTime(0.0f), m_lastFrame(0.0f)
	{
		m_uiManager = std::make_unique<UI::UIManager>(this);
	}

	// Test models
	std::shared_ptr<Rendering::Model> sphere;
	std::shared_ptr<Rendering::Model> cube;
	std::shared_ptr<Rendering::Model> track;

	Engine::Terrain::TerrainManager GEngine::terrainManager;


	bool GEngine::Initialize()
	{
		m_logger = spdlog::stdout_color_mt("engine");
		spdlog::set_level(spdlog::level::debug);
		spdlog::set_pattern("[%H:%M:%S.%e] [%^%l%$] [%s:%#] %v");

		SPDLOG_INFO("Starting Engine");

		// After window initialization, set the window reference in the camera
		if (!m_window.Initialize()) {
			return false;
		}

		Input::Initialize(m_window.GetNativeWindow());
		Input::SetCursorMode(GLFW_CURSOR_NORMAL);

		// Set the window reference in the camera
		m_camera.SetWindow(&m_window);

		// Initialize animation manager
		if (!m_animationManager->Initialize(&m_camera)) {
			spdlog::error("Failed to initialize animation manager");
			return false;
		}

		if (!InitializeRenderer() || !m_soundManager->Initialize()) {
			return false;
		}

		PhysicsManager::Initialize();
		m_uiManager->Initialize();


		m_particleManager = std::make_unique<ParticleManager>();
		if (!m_particleManager->Initialize(8000)) {
			SPDLOG_INFO("Failed to initalize particle manager");
			return false;
		}
		else {
			SPDLOG_INFO("good");
		}


		std::shared_ptr<Engine::Texture> tex1 = std::make_shared<Engine::Texture>();
		tex1->LoadFromFile("resources/textures/Terrain Grass.png");

		std::shared_ptr<Engine::Texture> tex2 = std::make_shared<Engine::Texture>();
		tex2->LoadFromFile("resources/textures/Terrain Dirt.png");

		std::shared_ptr<Engine::Texture> tex3 = std::make_shared<Engine::Texture>();
		tex3->LoadFromFile("resources/textures/Terrain Sand.png");

		std::shared_ptr<Engine::Texture> tex4 = std::make_shared<Engine::Texture>();
		tex4->LoadFromFile("resources/textures/Terrain Rock.png");

		std::shared_ptr<Engine::Texture> tex5 = std::make_shared<Engine::Texture>();
		tex5->LoadFromFile("resources/textures/white.png");


		terrainManager.LoadTerrainFile("resources/terrain/terrain1.bin");
		terrainManager.LoadTerrainFile("resources/terrain/terrain2.bin");
		terrainManager.GenerateMeshes();
		terrainManager.GenerateSplatTextures();
		terrainManager.SetupShaders();


		terrainManager.GetTerrains()[0]->diffuseTextures.push_back(tex1);
		terrainManager.GetTerrains()[0]->diffuseTextures.push_back(tex2);
		terrainManager.GetTerrains()[0]->diffuseTextures.push_back(tex3);
		terrainManager.GetTerrains()[0]->diffuseTextures.push_back(tex4);
		terrainManager.GetTerrains()[0]->diffuseTextures.push_back(tex5);

		terrainManager.GetTerrains()[1]->diffuseTextures.push_back(tex1);
		terrainManager.GetTerrains()[1]->diffuseTextures.push_back(tex2);
		terrainManager.GetTerrains()[1]->diffuseTextures.push_back(tex3);
		terrainManager.GetTerrains()[1]->diffuseTextures.push_back(tex4);
		terrainManager.GetTerrains()[1]->diffuseTextures.push_back(tex5);


		CreateInitialEntities();


		return true;
	}

	bool GEngine::InitializeRenderer()
	{
		m_renderer = std::make_unique<Renderer>(m_window, m_camera);
		if (!m_renderer->Initialize()) {
			m_logger->critical("Failed to initialize renderer");
			return false;
		}
		m_window.UpdateFramebufferSizes(m_window.GetWidth(), m_window.GetHeight());
		return true;
	}

	void GEngine::CreateInitialEntities()
	{
		BodyInterface& body_interface = PhysicsManager::GetPhysicsSystem()->GetBodyInterface();
		const RVec3    box_half_extents(0.5f, 0.5f, 0.5f);

		// Create physics body
		BodyCreationSettings cube_settings(new BoxShape(box_half_extents), RVec3(0.0, 5.0, 0.0), Quat::sIdentity(), EMotionType::Dynamic, Layers::MOVING);
		BodyID               cube_id = body_interface.CreateAndAddBody(cube_settings, EActivation::Activate);

		// Create floor
		BoxShapeSettings floor_shape_settings(Vec3(30.0f, 1.0f, 30.0f));
		floor_shape_settings.SetEmbedded(); // A ref counted object on the stack (base class
		                                    // RefTarget) should be marked as such to prevent it
		                                    // from being freed when its reference count goes to 0.
		ShapeSettings::ShapeResult floor_shape_result = floor_shape_settings.Create();
		ShapeRefC                  floor_shape        = floor_shape_result.Get(); // We don't expect an error here, but you can check
		                                                                          // floor_shape_result for HasError() / GetError()

		BodyCreationSettings floor_settings(floor_shape, RVec3(0.0_r, -1.0_r, 0.0_r), Quat::sIdentity(), EMotionType::Static, Layers::NON_MOVING);
		Body*                floor_body = body_interface.CreateBody(floor_settings); // Note that if we run out of bodies this can
		                                                                             // return nullptr
		body_interface.AddBody(floor_body->GetID(), EActivation::DontActivate);
		BodyID floor_id = floor_body->GetID();

		// Load models
		sphere = Rendering::ModelLoader::LoadModel("/home/gabe/CLionProjects/cpp-engine/resources/models/sphere.obj");
		cube   = Rendering::ModelLoader::LoadModel("/home/gabe/CLionProjects/cpp-engine/resources/models/cube.obj");


		// track = Rendering::ModelLoader::LoadModel("/home/gabe/Downloads/GCNCMarioCircuit/course_fix.dae");
		track = Rendering::ModelLoader::LoadModel("/home/gabe/Downloads/gltf/gltf/soccer_ball.gltf");


		std::shared_ptr<Rendering::Model> model = Rendering::ModelLoader::LoadModel("/home/gabe/CLionProjects/cpp-engine/resources/models/"
		                                                                            "TwistedTree_1.obj");

		// Create entities
		Entity floor = Entity::Create(this, "Track");
		floor.AddComponent<Components::ModelRenderer>(track);
		floor.AddComponent<Components::Transform>(glm::vec3(0.0f, -10.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.01f, 0.01f, 0.01f));
		floor.AddComponent<Components::RigidBodyComponent>(PhysicsManager::GetPhysicsSystem().get(), floor_id);

		//		Entity entity = Entity::Create(this, "TestEntity");
		//		entity.AddComponent<Components::ModelRenderer>(model);
		//		entity.AddComponent<Components::Transform>(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
		//		entity.AddComponent<Components::ParticleSystem>("/home/gabe/CLionProjects/cpp-engine/resources/particles/testleaf.efk");


		//		Entity entity2 = Entity::Create(this, "TestEntity2");
		//		entity2.AddComponent<Components::ModelRenderer>(model);
		//		entity2.AddComponent<Components::Transform>(glm::vec3(2.0f, 0.0f, -5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
		//		entity2.AddComponent<Components::RigidBodyComponent>(PhysicsManager::GetPhysicsSystem().get(), cube_id);
		//		entity2.AddComponent<Components::AudioSource>("birds", true, 0.1f, 1.0f, true, 5.0f, 50.0f, 1.0f);


		//		Entity animatedEntity = Entity::Create(this, "AnimatedEntity");
		//		animatedEntity.AddComponent<Components::Transform>(glm::vec3(0.0f, 3.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
		//		animatedEntity.AddComponent<Components::SkeletonComponent>("/home/gabe/CLionProjects/cpp-engine/resources/models/ruby_skeleton.ozz");
		//		animatedEntity.AddComponent<Components::AnimationComponent>("/home/gabe/CLionProjects/cpp-engine/resources/models/ruby_animation.ozz");
		//		animatedEntity.AddComponent<Components::AnimationPoseComponent>();
		//		animatedEntity.AddComponent<Components::AnimationWorkerComponent>();
		//		animatedEntity.AddComponent<Components::SkinnedMeshComponent>("/home/gabe/CLionProjects/cpp-engine/resources/models/ruby_mesh.ozz");

		//
		//		Terrain::TerrainTile tile = Terrain::LoadTerrainTile("resources/terrain/terrain.bin");
		//		std::cout << "Loaded tile: " << tile.name << "\n";
		//		std::cout << "Heights: " << tile.heightmap.size() << "\n";
		//		std::cout << "Trees: " << tile.trees.size() << "\n";

		//		auto& tile         = terrainManager.GetTerrains()[0];
		//		Body* terrain_body = body_interface.CreateBody(BodyCreationSettings(new MeshShapeSettings(tile->physicsMesh), RVec3(0.0_r, 0.0_r, 0.0_r), Quat::sIdentity(), EMotionType::Static, Layers::NON_MOVING));
		//		body_interface.AddBody(terrain_body->GetID(), EActivation::DontActivate);
	}

	void GEngine::Run()
	{
		while (!m_window.ShouldClose()) {
			auto currentFrame = static_cast<float>(glfwGetTime());
			m_deltaTime       = currentFrame - m_lastFrame;
			m_lastFrame       = currentFrame;

			Update();
		}
	}

	void GEngine::ProcessInput()
	{
		// Handle camera movement based on right mouse button state
		if (Input::IsMousePressed(GLFW_MOUSE_BUTTON_RIGHT)) {
			// Only capture cursor if it's not already captured
			if (Input::GetCursorMode() != GLFW_CURSOR_DISABLED) {
				Input::SetCursorMode(GLFW_CURSOR_DISABLED);
			}

			// Process camera movement with keyboard
			m_camera.ProcessKeyboard(m_deltaTime);

			// Process mouse movement for camera rotation
			glm::vec2 mouseDelta = Input::GetMouseDelta();
			m_camera.ProcessMouseMovement(mouseDelta.x, mouseDelta.y);
		}
		else {
			// Release cursor when right mouse button is released
			if (Input::GetCursorMode() == GLFW_CURSOR_DISABLED) {
				Input::SetCursorMode(GLFW_CURSOR_NORMAL);
			}
		}

		// Toggle physics when P is pressed
		if (Input::IsKeyPressedThisFrame(GLFW_KEY_P)) {
			m_physicsEnabled = !m_physicsEnabled;
			SPDLOG_INFO("Physics simulation {}", m_physicsEnabled ? "enabled" : "disabled");
		}

		// Process mouse scroll regardless of capture state
		float scrollDelta = Input::GetMouseScrollDelta();
		if (scrollDelta != 0.0f) {
			m_camera.ProcessMouseScroll(scrollDelta);
		}

		if (Input::IsKeyPressedThisFrame(GLFW_KEY_R)) {
			SPDLOG_INFO("r");
			// m_particleManager->SetPaused(handle, !m_particleManager->GetPaused(handle));
		}


		if (ImGui::IsKeyPressed(ImGuiKey_B)) {
			glm::vec3 cpos    = m_camera.GetPosition();
			glm::vec3 forward = m_camera.GetFront();

			BodyInterface&       body_interface = PhysicsManager::GetPhysicsSystem()->GetBodyInterface();
			BodyCreationSettings sphere_settings(new SphereShape(0.5f), RVec3(cpos.x, cpos.y, cpos.z), Quat::sIdentity(), EMotionType::Dynamic, Layers::MOVING);
			BodyID               sphere_id = body_interface.CreateAndAddBody(sphere_settings, EActivation::Activate);

			body_interface.AddLinearVelocity(sphere_id, RVec3(forward.x * 5, forward.y * 5, forward.z * 5));

			auto s = Entity::Create(this, "SphereEntity");
			s.AddComponent<Components::Transform>(glm::vec3(cpos.x, cpos.y, cpos.z), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
			s.AddComponent<Components::ModelRenderer>(sphere);
			// s.AddComponent<Components::ShadowCaster>();
			s.AddComponent<Components::RigidBodyComponent>(PhysicsManager::GetPhysicsSystem().get(), sphere_id);
		}

		if (ImGui::IsKeyPressed(ImGuiKey_C)) {
			const RVec3 box_half_extents(0.5f, 0.5f, 0.5f);
			glm::vec3   cpos    = m_camera.GetPosition();
			glm::vec3   forward = m_camera.GetFront();

			BodyInterface&       body_interface = PhysicsManager::GetPhysicsSystem()->GetBodyInterface();
			BodyCreationSettings cube_settings(new BoxShape(box_half_extents), RVec3(cpos.x, cpos.y, cpos.z), Quat::sIdentity(), EMotionType::Dynamic, Layers::MOVING);
			BodyID               cube_id = body_interface.CreateAndAddBody(cube_settings, EActivation::Activate);

			body_interface.AddLinearVelocity(cube_id, RVec3(forward.x * 5, forward.y * 5, forward.z * 5));

			auto s = Entity::Create(this, "CubeEntity");
			s.AddComponent<Components::Transform>(glm::vec3(cpos.x, cpos.y, cpos.z), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
			s.AddComponent<Components::ModelRenderer>(cube);
			// s.AddComponent<Components::ShadowCaster>();
			s.AddComponent<Components::RigidBodyComponent>(PhysicsManager::GetPhysicsSystem().get(), cube_id);
		}
	}


	void GEngine::Update()
	{
		ProcessInput();
		Engine::Window::PollEvents();
		Engine::Window::Update();
		Input::Update();


		m_animationManager->Update(m_deltaTime);

		if (m_physicsEnabled) PhysicsManager::Update(m_deltaTime);

		PhysicsManager::SyncPhysicsEntities(m_registry);

		m_soundManager->UpdateAudioEntities(m_registry, m_camera);


		m_renderer->RenderShadowMaps(m_registry);


		Engine::Window::GetFramebuffer(Window::FramebufferID::GAME_OUT)->Bind();
		m_renderer->PreRender();
		m_uiManager->BeginDockspace();


		m_renderer->RenderEntities(m_registry);

		terrainManager.Render(m_window, m_camera);

		m_animationManager->Render();

		m_renderer->RenderSkybox();

		m_particleManager->Update(m_registry, m_deltaTime);

		m_particleManager->Render(m_window, m_camera);

		Engine::Framebuffer::Unbind();

		m_uiManager->Render();
		m_uiManager->EndDockspace();
		m_renderer->PostRender();
	}

	void GEngine::Shutdown()
	{
		PhysicsManager::CleanUp(m_registry);

		// Shutdown sound manager
		if (m_soundManager) {
			m_soundManager->Shutdown();
		}


		Components::AnimationWorkerComponent::CleanAnimationContexts();
		Components::SkinnedMeshComponent::CleanSkinnedModels();
		Texture::CleanAllTextures();
		Rendering::Mesh::CleanAllMeshes();
		Engine::Window::Shutdown();
		SPDLOG_INFO("Engine shutdown complete");
	}
} // namespace Engine
