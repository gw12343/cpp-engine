#pragma once

#include <memory>
#include <string>
#define GLM_ENABLE_EXPERIMENTAL
#include "Jolt/Jolt.h"
#include "Jolt/Physics/Body/BodyActivationListener.h"
#include "Jolt/Physics/Body/BodyManager.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "rendering/Model.h"
#include "rendering/Shader.h"
#include "sound/SoundManager.h"
#include "spdlog/spdlog.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/containers/vector.h>
using namespace JPH;
using namespace JPH::literals;

namespace ozz::animation {
	class Skeleton;
	class Animation;

	class LocalToModelJob;
	class AnimationController;
} // namespace ozz::animation

namespace ozz::math {
	struct SoaTransform;
	struct Float4x4;
} // namespace ozz::math

namespace myns {
	class Mesh;
}

namespace Engine {

	// Forward declaration
	class Entity;

	// Component structs for the ECS system
	namespace Components {

		// Base Component class
		class Component {
		  public:
			Component()          = default;
			virtual ~Component() = default;

			virtual void OnAdded(Entity& entity) = 0;

			// New method for rendering component in inspector
			virtual void RenderInspector(Entity& entity) {}
		};

		// Basic metadata for an entity
		class EntityMetadata : public Component {
		  public:
			std::string name;
			std::string tag;
			bool        active = true;

			EntityMetadata() = default;
			EntityMetadata(const std::string& name) : name(name), tag("") {}
			EntityMetadata(const std::string& name, const std::string& tag) : name(name), tag(tag) {}

			void OnAdded(Entity& entity) override;
			void RenderInspector(Entity& entity) override;
		};

		// Transform component for positioning, rotating, and scaling entities
		class Transform : public Component {
		  public:
			glm::vec3 position = glm::vec3(0.0f);
			glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // Identity quaternion
			glm::vec3 scale    = glm::vec3(1.0f);

			Transform() = default;

			Transform(const glm::vec3& position) : position(position) {}

			Transform(const glm::vec3& position, const glm::vec3& eulerAngles = glm::vec3(0.0f), const glm::vec3& scale = glm::vec3(1.0f))
			    : position(position), rotation(glm::quat(glm::radians(eulerAngles))), scale(scale)
			{
			}

			// Set rotation using Euler angles (in degrees)
			void SetRotation(const glm::vec3& eulerAngles) { rotation = glm::quat(glm::radians(eulerAngles)); }

			// Get rotation as Euler angles (in degrees)
			glm::vec3 GetEulerAngles() const { return glm::degrees(glm::eulerAngles(rotation)); }

			// Get the transformation matrix
			glm::mat4 GetMatrix() const
			{
				glm::mat4 matrix = glm::mat4(1.0f);
				matrix           = glm::translate(matrix, position);
				matrix           = matrix * glm::toMat4(rotation);
				matrix           = glm::scale(matrix, scale);
				return matrix;
			}

			void OnAdded(Entity& entity) override;
			void RenderInspector(Entity& entity) override;
		};

		// Renderer component for 3D models
		class ModelRenderer : public Component {
		  public:
			std::shared_ptr<Model> model;
			bool                   visible = true;

			ModelRenderer() = default;

			ModelRenderer(const std::shared_ptr<Model>& model) : model(model) {}

			// Draw the model with the given shader and transform
			void Draw(const Shader& shader, const Transform& transform) const
			{
				if (visible && model) {
					// Set model matrix in shader
					shader.Use();
					glm::mat4 modelMatrix = transform.GetMatrix();
					shader.SetMat4("model", &modelMatrix);

					// Draw the model
					model->Draw(shader);
				}
			}

			void OnAdded(Entity& entity) override;
			void RenderInspector(Entity& entity) override;
		};

		class RigidBodyComponent : public Component {
		  public:
			JPH::PhysicsSystem* physicsSystem;
			JPH::BodyID         bodyID;

			RigidBodyComponent() = default;

			RigidBodyComponent(JPH::PhysicsSystem* physicsSystem, const JPH::BodyID& bodyID) : physicsSystem(physicsSystem), bodyID(bodyID) {}

			void OnAdded(Entity& entity) override;
			void RenderInspector(Entity& entity) override;
		};

		class AudioSource : public Component {
		  public:
			std::string soundName;
			bool        autoPlay  = false;
			bool        looping   = false;
			float       volume    = 1.0f;
			float       pitch     = 1.0f;
			bool        isPlaying = false;

			// Attenuation parameters
			float referenceDistance = 1.0f;   // Distance at which volume is at maximum
			float maxDistance       = 100.0f; // Distance at which attenuation stops
			float rolloffFactor     = 1.0f;   // How quickly the sound attenuates

			std::shared_ptr<Audio::SoundSource> source;

			AudioSource() = default;

			AudioSource(const std::string& name,
			            bool               loop    = false,
			            float              vol     = 1.0f,
			            float              p       = 1.0f,
			            bool               play    = false,
			            float              refDist = 1.0f,
			            float              maxDist = 100.0f,
			            float              rolloff = 1.0f)
			    : soundName(name), looping(loop), volume(vol), pitch(p), autoPlay(play), referenceDistance(refDist), maxDistance(maxDist),
			      rolloffFactor(rolloff)
			{
				source = std::make_shared<Audio::SoundSource>(looping);
				source->SetGain(volume);
				source->SetPitch(pitch);

				// Explicitly configure attenuation parameters
				source->ConfigureAttenuation(referenceDistance, maxDistance, rolloffFactor);

				// Log the configuration
				spdlog::info("Created AudioSource with attenuation: ref={}, max={}, rolloff={}", referenceDistance, maxDistance, rolloffFactor);
			}

			void Play(Audio::SoundManager& soundManager)
			{
				if (!source) return;

				auto buffer = soundManager.GetSound(soundName);
				if (buffer) {
					source->Play(*buffer);
					isPlaying = true;
				}
			}

			void Stop()
			{
				if (source) {
					source->Stop();
					isPlaying = false;
				}
			}

			void OnAdded(Entity& entity) override;
			void RenderInspector(Entity& entity) override;
		};

		class SkeletonComponent : public Component {
		  public:
			ozz::animation::Skeleton* skeleton = nullptr;
			std::string               skeletonPath;

			SkeletonComponent() = default;
			SkeletonComponent(ozz::animation::Skeleton* skeleton) : skeleton(skeleton) {}
			SkeletonComponent(std::string skeletonPath) : skeletonPath(skeletonPath) {}

			void OnAdded(Entity& entity) override;
			void RenderInspector(Entity& entity) override;
		};

		class AnimationComponent : public Component {
		  public:
			ozz::animation::Animation* animation = nullptr;
			std::string                animationPath;

			AnimationComponent() = default;
			AnimationComponent(ozz::animation::Animation* animation) : animation(animation) {}
			AnimationComponent(std::string animationPath) : animationPath(animationPath) {}
			void OnAdded(Entity& entity) override;
			void RenderInspector(Entity& entity) override;
		};

		class AnimationPoseComponent : public Component {
		  public:
			std::vector<ozz::math::SoaTransform>* local_pose = nullptr;
			std::vector<ozz::math::Float4x4>*     model_pose = nullptr;

			AnimationPoseComponent() = default;

			void OnAdded(Entity& entity) override;
			void RenderInspector(Entity& entity) override;
		};

		class AnimationWorkerComponent : public Component {
		  public:
			ozz::animation::SamplingJob::Context* context = nullptr;

			AnimationWorkerComponent() = default;

			void OnAdded(Entity& entity) override;
			void RenderInspector(Entity& entity) override;
		};

		class SkinnedMeshComponent : public Component {
		  public:
			ozz::vector<myns::Mesh>*          meshes            = nullptr;
			std::vector<ozz::math::Float4x4>* skinning_matrices = nullptr;
			std::string                       meshPath;

			SkinnedMeshComponent() = default;
			SkinnedMeshComponent(ozz::vector<myns::Mesh>* meshes) : meshes(meshes) {}
			SkinnedMeshComponent(std::string meshPath) : meshPath(meshPath) {}

			void OnAdded(Entity& entity) override;
			void RenderInspector(Entity& entity) override;
		};


	} // namespace Components
} // namespace Engine