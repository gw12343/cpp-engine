#include "physics/PhysicsManager.h"

#include "components/Components.h"
#include "physics/PhysicsInterfaces.h"

#include <cstdarg>


using namespace JPH;
using namespace JPH::literals;

namespace Engine {

	BPLayerInterfaceImpl              PhysicsManager::broad_phase_layer_interface;
	ObjectVsBroadPhaseLayerFilterImpl PhysicsManager::object_vs_broadphase_layer_filter;
	ObjectLayerPairFilterImpl         PhysicsManager::object_vs_object_layer_filter;
	MyContactListener                 PhysicsManager::contact_listener;
	MyBodyActivationListener          PhysicsManager::body_activation_listener;


	void PhysicsManager::TraceImpl(const char* inFMT, ...)
	{
		// Format the message
		va_list list;
		va_start(list, inFMT);
		char buffer[1024];
		vsnprintf(buffer, sizeof(buffer), inFMT, list);
		va_end(list);

		// Print to the TTY
		spdlog::error("Trace: {0}", buffer);
	}
	// Callback for asserts, connect this to your own assert handler if you have one
	bool PhysicsManager::AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, uint inLine)
	{
		spdlog::error("{0}:{1}: (got: {2}) {3}", inFile, inLine, inExpression, (inMessage != nullptr ? inMessage : ""));

		// Breakpoint
		return true;
	};


	void PhysicsManager::Initialize(std::shared_ptr<PhysicsSystem>       physics_system,
	                                std::shared_ptr<TempAllocatorImpl>   allocater,
	                                std::shared_ptr<JobSystemThreadPool> jobs)
	{
		physics_system->Init(cMaxBodies,
		                     cNumBodyMutexes,
		                     cMaxBodyPairs,
		                     cMaxContactConstraints,
		                     broad_phase_layer_interface,
		                     object_vs_broadphase_layer_filter,
		                     object_vs_object_layer_filter);

		// Set the contact listener
		physics_system->SetContactListener(&contact_listener);

		// Set the body activation listener
		physics_system->SetBodyActivationListener(&body_activation_listener);
	}

	void PhysicsManager::Update(std::shared_ptr<PhysicsSystem>       physics,
	                            std::shared_ptr<TempAllocatorImpl>   allocater,
	                            std::shared_ptr<JobSystemThreadPool> jobs,
	                            float                                dt)
	{
		// Collision Steps to simulate. Should be around 1 per 16ms
		int cCollisionSteps = static_cast<int>(glm::ceil(dt * 60.0f));
		// Step the world
		physics->Update(dt, cCollisionSteps, allocater.get(), jobs.get());
	}


	void PhysicsManager::CleanUp(entt::registry& registry, std::shared_ptr<PhysicsSystem> physics)
	{
		spdlog::info("cleaning up physics");

		auto           physicsView    = registry.view<Engine::Components::RigidBodyComponent>();
		BodyInterface& body_interface = physics->GetBodyInterface();

		for (auto [entity, rb] : physicsView.each()) {
			body_interface.RemoveBody(rb.bodyID);
			body_interface.DestroyBody(rb.bodyID);
		}

		UnregisterTypes();

		// Destroy the factory
		delete Factory::sInstance;
		Factory::sInstance = nullptr;
	}

	void DecomposeMatrix(const JPH::RMat44& mat, glm::vec3& position, glm::quat& rotation, glm::vec3& scale)
	{
		// Convert Jolt matrix to glm matrix
		glm::mat4 glmMat;
		glmMat[0][0] = mat(0, 0);
		glmMat[1][0] = mat(0, 1);
		glmMat[2][0] = mat(0, 2);
		glmMat[3][0] = mat(0, 3);

		glmMat[0][1] = mat(1, 0);
		glmMat[1][1] = mat(1, 1);
		glmMat[2][1] = mat(1, 2);
		glmMat[3][1] = mat(1, 3);

		glmMat[0][2] = mat(2, 0);
		glmMat[1][2] = mat(2, 1);
		glmMat[2][2] = mat(2, 2);
		glmMat[3][2] = mat(2, 3);

		glmMat[0][3] = mat(3, 0);
		glmMat[1][3] = mat(3, 1);
		glmMat[2][3] = mat(3, 2);
		glmMat[3][3] = mat(3, 3);

		// Extract scale
		glm::vec3 scaleX(glm::length(glmMat[0]));
		glm::vec3 scaleY(glm::length(glmMat[1]));
		glm::vec3 scaleZ(glm::length(glmMat[2]));
		scale = glm::vec3(scaleX.x, scaleY.x, scaleZ.x);

		// Remove scale from the matrix
		glmMat[0] = glmMat[0] / scale.x;
		glmMat[1] = glmMat[1] / scale.y;
		glmMat[2] = glmMat[2] / scale.z;

		// Extract rotation
		glm::mat3 rotationMatrix(glmMat);
		glm::quat glmQuat = glm::quat_cast(rotationMatrix);
		rotation          = glmQuat;

		// Extract translation
		position = glm::vec3(glmMat[3][0], glmMat[3][1], glmMat[3][2]);
	}

	glm::mat4 CalculateModelMatrix(const Engine::Components::Transform& transform)
	{
		// Create the translation matrix
		glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), transform.position);

		// Create the rotation matrix from quaternion
		glm::mat4 rotationMatrix = glm::mat4_cast(transform.rotation);

		// Create the scale matrix
		glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), transform.scale);

		// Combine the matrices: Scale * Rotate * Translate
		glm::mat4 modelMatrix = translationMatrix * rotationMatrix * scaleMatrix;

		return modelMatrix;
	}

	void PhysicsManager::UpdatePhysicsEntities(entt::registry& registry, std::shared_ptr<PhysicsSystem> physics)
	{
		auto           physicsView    = registry.view<Engine::Components::Transform, Engine::Components::RigidBodyComponent>();
		BodyInterface& body_interface = physics->GetBodyInterface();

		for (auto [entity, transform, rb] : physicsView.each()) {
			RMat44    tform = body_interface.GetCenterOfMassTransform(rb.bodyID);
			glm::vec3 scl;
			DecomposeMatrix(tform, transform.position, transform.rotation, scl);
			Vec3 velocity = body_interface.GetLinearVelocity(rb.bodyID);
		}
	}
} // namespace Engine