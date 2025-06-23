#include "Engine.h"

#include "components/Components.h"
#include "terrain/TerrainLoader.h"
#include "utils/ModelLoader.h"
#include "Jolt/Physics/Collision/Shape/MeshShape.h"
#include "core/module/ModuleManager.h"
#include "core/module/TestModule.h"
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


using namespace JPH;
using namespace JPH::literals;

namespace Engine {
	ModuleManager manager;


	GEngine::GEngine(int width, int height, const char* title) : m_deltaTime(0.0f), m_lastFrame(0.0f)
	{
		Get().registry = std::make_shared<entt::registry>();

		// Initialize Modules
		Get().window    = std::make_shared<Window>(width, height, title);
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

	// Test models
	std::shared_ptr<Rendering::Model> sphere;
	std::shared_ptr<Rendering::Model> cube;


	bool GEngine::Initialize()
	{
		SPDLOG_INFO("Starting Engine");
		manager.InitAll();

		Input::Initialize(GetWindow().GetNativeWindow());
		Input::SetCursorMode(GLFW_CURSOR_NORMAL);


		CreateInitialEntities();
		return true;
	}

	void GEngine::CreateInitialEntities()
	{
		BodyInterface& body_interface = GetPhysics().GetPhysicsSystem()->GetBodyInterface();
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


		// track = Rendering::ModelLoader::LoadModel("/home/gabe/Downloads/GCNCMarioCircuit/course_fix.dae");
		// track = Rendering::ModelLoader::LoadModel("/home/gabe/Downloads/gltf/gltf/soccer_ball.gltf");


		std::shared_ptr<Rendering::Model> model = Rendering::ModelLoader::LoadModel("/home/gabe/CLionProjects/cpp-engine/resources/models/"
		                                                                            "TwistedTree_1.obj");

		cube = Rendering::ModelLoader::LoadModel("/home/gabe/CLionProjects/cpp-engine/resources/models/cube.obj");
		// Create entities
		Entity floor = Entity::Create("TestCube");
		floor.AddComponent<Components::ModelRenderer>(cube);
		floor.AddComponent<Components::Transform>(glm::vec3(0.0f, -10.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.01f, 0.01f, 0.01f));
		floor.AddComponent<Components::RigidBodyComponent>(GetPhysics().GetPhysicsSystem().get(), floor_id);

		Entity entity = Entity::Create("TestEntity");
		entity.AddComponent<Components::ModelRenderer>(model);
		entity.AddComponent<Components::Transform>(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
		entity.AddComponent<Components::ParticleSystem>("/home/gabe/CLionProjects/cpp-engine/resources/particles/testleaf.efk");


		Entity entity2 = Entity::Create("TestEntity2");
		entity2.AddComponent<Components::ModelRenderer>(model);
		entity2.AddComponent<Components::Transform>(glm::vec3(2.0f, 0.0f, -5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
		entity2.AddComponent<Components::RigidBodyComponent>(GetPhysics().GetPhysicsSystem().get(), cube_id);
		entity2.AddComponent<Components::AudioSource>("birds", true, 0.1f, 1.0f, true, 5.0f, 50.0f, 1.0f);


		Entity animatedEntity = Entity::Create("AnimatedEntity");
		animatedEntity.AddComponent<Components::Transform>(glm::vec3(0.0f, 3.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
		animatedEntity.AddComponent<Components::SkeletonComponent>("/home/gabe/CLionProjects/cpp-engine/resources/models/ruby_skeleton.ozz");
		animatedEntity.AddComponent<Components::AnimationComponent>("/home/gabe/CLionProjects/cpp-engine/resources/models/ruby_animation.ozz");
		animatedEntity.AddComponent<Components::AnimationPoseComponent>();
		animatedEntity.AddComponent<Components::AnimationWorkerComponent>();
		animatedEntity.AddComponent<Components::SkinnedMeshComponent>("/home/gabe/CLionProjects/cpp-engine/resources/models/ruby_mesh.ozz");

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
		while (!GetWindow().ShouldClose()) {
			auto currentFrame = static_cast<float>(glfwGetTime());
			m_deltaTime       = currentFrame - m_lastFrame;
			m_lastFrame       = currentFrame;

			Update();
		}
	}

	void GEngine::ProcessInput() const
	{
		// Handle camera movement based on right mouse button state
		if (Input::IsMousePressed(GLFW_MOUSE_BUTTON_RIGHT)) {
			// Only capture cursor if it's not already captured
			if (Input::GetCursorMode() != GLFW_CURSOR_DISABLED) {
				Input::SetCursorMode(GLFW_CURSOR_DISABLED);
			}

			// Process camera movement with keyboard
			GetCamera().ProcessKeyboard(m_deltaTime);

			// Process mouse movement for camera rotation
			glm::vec2 mouseDelta = Input::GetMouseDelta();
			GetCamera().ProcessMouseMovement(mouseDelta.x, mouseDelta.y);
		}
		else {
			// Release cursor when right mouse button is released
			if (Input::GetCursorMode() == GLFW_CURSOR_DISABLED) {
				Input::SetCursorMode(GLFW_CURSOR_NORMAL);
			}
		}

		// Toggle physics when P is pressed
		if (Input::IsKeyPressedThisFrame(GLFW_KEY_P)) {
			Get().isPhysicsPaused = !Get().isPhysicsPaused;
			SPDLOG_INFO("Physics simulation {}", Get().isPhysicsPaused ? "enabled" : "disabled");
		}

		// Process mouse scroll regardless of capture state
		float scrollDelta = Input::GetMouseScrollDelta();
		if (scrollDelta != 0.0f) {
			GetCamera().ProcessMouseScroll(scrollDelta);
		}

		//		if (Input::IsKeyPressedThisFrame(GLFW_KEY_R)) {
		//			SPDLOG_INFO("r");
		//			// m_particleManager->SetPaused(handle, !m_particleManager->GetPaused(handle));
		//		}
		//
		//
		if (ImGui::IsKeyPressed(ImGuiKey_B)) {
			glm::vec3 cpos    = GetCamera().GetPosition();
			glm::vec3 forward = GetCamera().GetFront();

			BodyInterface&       body_interface = GetPhysics().GetPhysicsSystem()->GetBodyInterface();
			BodyCreationSettings sphere_settings(new SphereShape(0.5f), RVec3(cpos.x, cpos.y, cpos.z), Quat::sIdentity(), EMotionType::Dynamic, Layers::MOVING);
			BodyID               sphere_id = body_interface.CreateAndAddBody(sphere_settings, EActivation::Activate);

			body_interface.AddLinearVelocity(sphere_id, RVec3(forward.x * 5, forward.y * 5, forward.z * 5));

			auto s = Entity::Create("SphereEntity");
			s.AddComponent<Components::Transform>(glm::vec3(cpos.x, cpos.y, cpos.z), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
			s.AddComponent<Components::ModelRenderer>(sphere);
			// s.AddComponent<Components::ShadowCaster>();
			s.AddComponent<Components::RigidBodyComponent>(GetPhysics().GetPhysicsSystem().get(), sphere_id);
		}

		if (ImGui::IsKeyPressed(ImGuiKey_C)) {
			const RVec3 box_half_extents(0.5f, 0.5f, 0.5f);
			glm::vec3   cpos    = GetCamera().GetPosition();
			glm::vec3   forward = GetCamera().GetFront();

			BodyInterface&       body_interface = GetPhysics().GetPhysicsSystem()->GetBodyInterface();
			BodyCreationSettings cube_settings(new BoxShape(box_half_extents), RVec3(cpos.x, cpos.y, cpos.z), Quat::sIdentity(), EMotionType::Dynamic, Layers::MOVING);
			BodyID               cube_id = body_interface.CreateAndAddBody(cube_settings, EActivation::Activate);

			body_interface.AddLinearVelocity(cube_id, RVec3(forward.x * 5, forward.y * 5, forward.z * 5));

			auto s = Entity::Create("CubeEntity");
			s.AddComponent<Components::Transform>(glm::vec3(cpos.x, cpos.y, cpos.z), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
			s.AddComponent<Components::ModelRenderer>(cube);
			// s.AddComponent<Components::ShadowCaster>();
			s.AddComponent<Components::RigidBodyComponent>(GetPhysics().GetPhysicsSystem().get(), cube_id);
		}
	}


	void GEngine::Update()
	{
		Window::PollEvents();
		ProcessInput();
		Input::Update();
		manager.UpdateAll(m_deltaTime);


		//		m_animationManager->Update(m_deltaTime);
		//		if (m_physicsEnabled) PhysicsManager::Update(m_deltaTime);
		//		PhysicsManager::SyncPhysicsEntities(m_registry);
		//		m_soundManager->UpdateAudioEntities(m_registry, m_camera);

		//
		//		m_renderer->PreRender();
		//		m_uiManager->BeginDockspace();
		//		m_uiManager->Render();
		//		m_uiManager->EndDockspace();
		//
		//		terrainManager.Render(m_window, m_camera);
		//		m_animationManager->Render();
		//		m_particleManager->Update(m_registry, m_deltaTime);
		//		m_particleManager->Render(m_window, m_camera);
		//
		//		m_renderer->RenderEntities(m_registry);
		//
		//
		//
		//
		//

		//		m_renderer->RenderShadowMaps(m_registry);
		//		Engine::Window::GetFramebuffer(Window::FramebufferID::GAME_OUT)->Bind();
		//		m_renderer->RenderSkybox();
		//		Engine::Framebuffer::Unbind();
		//		m_renderer->PostRender();
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
