#include "Engine.h"

#include "Entity.h"
#include "Input.h"
#include "components/Components.h"
#include "physics/PhysicsManager.h"
#include "utils/ModelLoader.h"

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>
#include <fstream>
#include <imgui.h>
#include <iostream>
#include <memory>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <string>

using namespace JPH;
using namespace JPH::literals;


namespace Engine {

	GEngine::GEngine(int width, int height, const char* title)
	    : m_window(width, height, title), m_camera(glm::vec3(0.0f, 3.0f, 6.0f), glm::vec3(0, 1, 0), -90.0f, -30.0f),
	      m_soundManager(std::make_shared<Audio::SoundManager>()),
	      m_animationManager(std::make_unique<AnimationManager>()), m_deltaTime(0.0f), m_lastFrame(0.0f)
	{
		m_uiManager = std::make_unique<UIManager>(this);
	}
	GEngine::~GEngine()
	{
		Shutdown();
	}

	// Physics system
	std::shared_ptr<PhysicsSystem>       physics;
	std::shared_ptr<TempAllocatorImpl>   allocater;
	std::shared_ptr<JobSystemThreadPool> jobs;

	// Audio system
	std::shared_ptr<Audio::SoundBuffer> m_backgroundMusic;
	std::shared_ptr<Audio::SoundSource> m_backgroundMusicSource;
	Entity                              m_selectedEntity;
	ozz::unique_ptr<RendererImpl>       renderer_;

	// Test models
	std::shared_ptr<Model> sphere;
	std::shared_ptr<Model> cube;

	bool GEngine::Initialize()
	{
		m_logger = spdlog::stdout_color_mt("engine");
		spdlog::set_level(spdlog::level::debug);
		spdlog::set_pattern("[%H:%M:%S.%e] [%^%l%$] [%s:%#] %v");
		spdlog::info("Starting Engine");

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

		if (!InitializeRenderer() || !m_soundManager->Initialize() || !InitializePhysics()) {
			return false;
		}

		m_uiManager->Initialize();

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
		return true;
	}

	bool GEngine::InitializePhysics()
	{
		RegisterDefaultAllocator();
		Trace = PhysicsManager::TraceImpl;
		JPH_IF_ENABLE_ASSERTS(AssertFailed = PhysicsManager::AssertFailedImpl;)
		Factory::sInstance = new Factory();
		RegisterTypes();

		allocater = std::make_shared<TempAllocatorImpl>(10 * 1024 * 1024);
		jobs      = std::make_shared<JobSystemThreadPool>(
            cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);
		physics = std::make_shared<PhysicsSystem>();

		PhysicsManager::Initialize(physics, allocater, jobs);
		return true;
	}

	void GEngine::CreateInitialEntities()
	{
		BodyInterface& body_interface = physics->GetBodyInterface();
		const RVec3    box_half_extents(0.5f, 0.5f, 0.5f);

		// Create physics body
		BodyCreationSettings cube_settings(new BoxShape(box_half_extents),
		                                   RVec3(0.0, 5.0, 0.0),
		                                   Quat::sIdentity(),
		                                   EMotionType::Dynamic,
		                                   Layers::MOVING);
		BodyID               cube_id = body_interface.CreateAndAddBody(cube_settings, EActivation::Activate);

		// Create floor
		BoxShapeSettings floor_shape_settings(Vec3(30.0f, 1.0f, 30.0f));
		floor_shape_settings.SetEmbedded(); // A ref counted object on the stack (base class
		                                    // RefTarget) should be marked as such to prevent it
		                                    // from being freed when its reference count goes to 0.
		ShapeSettings::ShapeResult floor_shape_result = floor_shape_settings.Create();
		ShapeRefC floor_shape = floor_shape_result.Get(); // We don't expect an error here, but you can check
		                                                  // floor_shape_result for HasError() / GetError()

		BodyCreationSettings floor_settings(
		    floor_shape, RVec3(0.0_r, -1.0_r, 0.0_r), Quat::sIdentity(), EMotionType::Static, Layers::NON_MOVING);
		Body* floor_body = body_interface.CreateBody(floor_settings); // Note that if we run out of bodies this can
		                                                              // return nullptr
		body_interface.AddBody(floor_body->GetID(), EActivation::DontActivate);
		BodyID floor_id = floor_body->GetID();

		// Load models
		sphere = ModelLoader::LoadModel("/home/gabe/CLionProjects/cpp-engine/resources/models/sphere.obj");
		cube   = ModelLoader::LoadModel("/home/gabe/CLionProjects/cpp-engine/resources/models/cube.obj");
		std::shared_ptr<Model> model = ModelLoader::LoadModel("/home/gabe/CLionProjects/cpp-engine/resources/models/"
		                                                      "TwistedTree_1.obj");

		// Create entities
		Entity floor = Entity::Create(this, "FloorEntity");
		floor.AddComponent<Components::ModelRenderer>(cube);
		floor.AddComponent<Components::Transform>(
		    glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(60.0f, 2.0f, 60.0f));
		floor.AddComponent<Components::RigidBodyComponent>(physics.get(), floor_id);

		Entity entity = Entity::Create(this, "TestEntity");
		entity.AddComponent<Components::ModelRenderer>(cube);
		entity.AddComponent<Components::Transform>(
		    glm::vec3(0.0f, -5.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));

		Entity entity2 = Entity::Create(this, "TestEntity2");
		entity2.AddComponent<Components::ModelRenderer>(model);
		entity2.AddComponent<Components::Transform>(
		    glm::vec3(2.0f, 0.0f, -5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
		entity2.AddComponent<Components::RigidBodyComponent>(physics.get(), cube_id);
		entity2.AddComponent<Components::AudioSource>("birds", true, 0.1f, 1.0f, true, 5.0f, 50.0f, 1.0f);
	}


	void GEngine::Run()
	{
		while (!m_window.ShouldClose()) {
			float currentFrame = glfwGetTime();
			m_deltaTime        = currentFrame - m_lastFrame;
			m_lastFrame        = currentFrame;

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
			spdlog::info("Physics simulation {}", m_physicsEnabled ? "enabled" : "disabled");
		}

		// Process mouse scroll regardless of capture state
		float scrollDelta = Input::GetMouseScrollDelta();
		if (scrollDelta != 0.0f) {
			m_camera.ProcessMouseScroll(scrollDelta);
		}

		if (ImGui::IsKeyPressed(ImGuiKey_B)) {
			float     r       = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
			glm::vec3 cpos    = m_camera.GetPosition();
			glm::vec3 forward = m_camera.GetFront();

			BodyInterface&       body_interface = physics->GetBodyInterface();
			BodyCreationSettings sphere_settings(new SphereShape(0.5f),
			                                     RVec3(cpos.x, cpos.y, cpos.z),
			                                     Quat::sIdentity(),
			                                     EMotionType::Dynamic,
			                                     Layers::MOVING);
			BodyID               sphere_id = body_interface.CreateAndAddBody(sphere_settings, EActivation::Activate);

			body_interface.AddLinearVelocity(sphere_id, RVec3(forward.x * 5, forward.y * 5, forward.z * 5));

			auto s = Entity::Create(this, "SphereEntity");
			s.AddComponent<Components::Transform>(
			    glm::vec3(cpos.x, cpos.y, cpos.z), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
			s.AddComponent<Components::ModelRenderer>(sphere);
			s.AddComponent<Components::RigidBodyComponent>(physics.get(), sphere_id);
		}

		if (ImGui::IsKeyPressed(ImGuiKey_C)) {
			const RVec3 box_half_extents(0.5f, 0.5f, 0.5f);
			float       r       = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
			glm::vec3   cpos    = m_camera.GetPosition();
			glm::vec3   forward = m_camera.GetFront();

			BodyInterface&       body_interface = physics->GetBodyInterface();
			BodyCreationSettings cube_settings(new BoxShape(box_half_extents),
			                                   RVec3(cpos.x, cpos.y, cpos.z),
			                                   Quat::sIdentity(),
			                                   EMotionType::Dynamic,
			                                   Layers::MOVING);
			BodyID               cube_id = body_interface.CreateAndAddBody(cube_settings, EActivation::Activate);

			body_interface.AddLinearVelocity(cube_id, RVec3(forward.x * 1, forward.y * 1, forward.z * 1));

			auto s = Entity::Create(this, "CubeEntity");
			s.AddComponent<Components::Transform>(
			    glm::vec3(cpos.x, cpos.y, cpos.z), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
			s.AddComponent<Components::ModelRenderer>(cube);
			s.AddComponent<Components::RigidBodyComponent>(physics.get(), cube_id);
		}
	}

	void GEngine::UpdatePhysics()
	{
		if (m_physicsEnabled) PhysicsManager::Update(physics, allocater, jobs, m_deltaTime);

		PhysicsManager::UpdatePhysicsEntities(m_registry, physics);
	}

	void GEngine::Update()
	{
		ProcessInput();
		m_window.PollEvents();
		m_window.Update();
		Input::Update();
		m_renderer->PreRender();

		// Update animation
		m_animationManager->Update(m_deltaTime);

		UpdatePhysics();
		m_soundManager->UpdateAudioEntities(m_registry, m_camera);
		// Use the renderer to render entities
		m_renderer->RenderEntities(m_registry);

		// Render animated models
		m_animationManager->Render();

		m_uiManager->Render();
		m_renderer->PostRender();
	}

	void GEngine::Shutdown()
	{
		renderer_.reset();
		PhysicsManager::CleanUp(m_registry, physics);
		// Stop background music if playing
		if (m_backgroundMusicSource) {
			m_backgroundMusicSource->Stop();
			m_backgroundMusicSource.reset();
		}

		// Clear background music buffer
		m_backgroundMusic.reset();
		// Shutdown sound manager
		if (m_soundManager) {
			m_soundManager->Shutdown();
		}

		m_logger->info("Engine shutdown complete");
	}
} // namespace Engine
