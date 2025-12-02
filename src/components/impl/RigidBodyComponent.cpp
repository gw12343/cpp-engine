//
// Created by gabe on 6/24/25.
//

#include "assets/AssetManager.h"
#include "assets/AssetManager.h"
#include "components/Components.h"

#include "core/Entity.h"
#include "utils/Utils.h"

#include "imgui.h"
#include "ozz/animation/runtime/track.h"
#include "rendering/particles/ParticleManager.h"
#include "animation/AnimationManager.h"
#include "scripting/ScriptManager.h"
#include "physics/PhysicsManager.h"
#include "RigidBodyComponent.h"
#include "rendering/ui/InspectorUI.h"

namespace Engine::Components {


	void RigidBodyComponent::OnRemoved(Entity& entity)
	{
		GetPhysics().GetPhysicsSystem()->GetBodyInterface().RemoveBody(bodyID);
		GetPhysics().bodyToEntityMap.erase(bodyID);
	}
	void RigidBodyComponent::OnAdded(Entity& entity)
	{
		BodyInterface& body_interface = GetPhysics().GetPhysicsSystem()->GetBodyInterface();

		if (!body_interface.IsAdded(bodyID)) {
			JPH::Ref<JPH::Shape> shape;
			if (shapeType == "Box") {
				shape = new JPH::BoxShape(shapeSize);
			}
			else if (shapeType == "Sphere") {
				shape = new JPH::SphereShape(shapeSize.GetX());
			}
			else if (shapeType == "Capsule") {
				shape = new JPH::CapsuleShape(shapeSize.GetX(), shapeSize.GetY());
			}
			else if (shapeType == "Cylinder") {
				shape = new JPH::CylinderShape(shapeSize.GetX(), shapeSize.GetY());
			}
			else if (shapeType == "ConvexMesh") {
				// Try to create convex mesh shape from model
				GetPhysics().log->info("Creating convex mesh shape from model {}", colliderModel.GetID());
				if (colliderModel.IsValid()) {
					auto* model = GetAssetManager().Get(colliderModel);
					if (model) {
						JPH::Array<JPH::Vec3> points;
						const auto&           meshes = model->GetMeshes();

						// Ensure mesh selection matches model
						if (meshSelection.size() != meshes.size()) {
							GetPhysics().log->warn("Mesh selection size does not match model size, resizing to match. Was {} now {}", meshSelection.size(), meshes.size());
							meshSelection.resize(meshes.size(), true);
						}

						int meshIndex = 0;
						for (const auto& mesh : meshes) {
							if (!meshSelection[meshIndex]) {
								meshIndex++;
								continue;
							}
							const auto& vertices = mesh->GetVertices();

							// Add all vertices as points for convex hull
							for (const auto& vertex : vertices) {
								points.push_back(JPH::Vec3(vertex.Position.x, vertex.Position.y, vertex.Position.z));
							}
							meshIndex++;
						}

						if (!points.empty()) {
							JPH::ConvexHullShapeSettings convexSettings(points);
							auto                         convexResult = convexSettings.Create();
							if (!convexResult.HasError()) {
								// Store the center of mass offset for later use in physics sync
								centerOfMassOffset = convexResult.Get()->GetCenterOfMass();
								shape              = convexResult.Get();
							}
							else {
								GetPhysics().log->warn("Failed to create convex hull, falling back to box collider");
								shape     = new JPH::BoxShape(shapeSize);
								shapeType = "Box";
							}
						}
						else {
							GetPhysics().log->warn("Failed to extract vertex data, falling back to box collider");
							shape     = new JPH::BoxShape(shapeSize);
							shapeType = "Box";
						}
					}
					else {
						GetPhysics().log->warn("ModelRenderer model could not be loaded, falling back to box collider");
						shape     = new JPH::BoxShape(shapeSize);
						shapeType = "Box";
					}
				}
				else {
					GetPhysics().log->warn("ModelRenderer has no valid model, falling back to box collider");
					shape     = new JPH::BoxShape(shapeSize);
					shapeType = "Box";
				}
			}

			else if (shapeType == "Mesh") {
				GetPhysics().log->info("Creating mesh shape from model {}", colliderModel.GetID());
				// Try to create mesh shape from model
				if (colliderModel.IsValid()) {
					auto* model = GetAssetManager().Get(colliderModel);
					if (model) {
						JPH::TriangleList triangles;
						const auto&       meshes = model->GetMeshes();

						// Ensure mesh selection matches model
						if (meshSelection.size() != meshes.size()) {
							GetPhysics().log->warn("Mesh selection size does not match model size, resizing to match. Was {} now {}", meshSelection.size(), meshes.size());
							meshSelection.resize(meshes.size(), true);
						}

						int meshIndex = 0;
						for (const auto& mesh : meshes) {
							if (!meshSelection[meshIndex]) {
								meshIndex++;
								continue;
							}
							const auto& vertices = mesh->GetVertices();
							const auto& indices  = mesh->GetIndices();

							// Convert indices to triangles
							for (size_t i = 0; i < indices.size(); i += 3) {
								if (i + 2 < indices.size()) {
									const auto& v0 = vertices[indices[i]].Position;
									const auto& v1 = vertices[indices[i + 1]].Position;
									const auto& v2 = vertices[indices[i + 2]].Position;

									JPH::Triangle triangle(JPH::Float3(v0.x, v0.y, v0.z), JPH::Float3(v1.x, v1.y, v1.z), JPH::Float3(v2.x, v2.y, v2.z));
									triangles.push_back(triangle);
								}
							}
							meshIndex++;
						}

						if (!triangles.empty()) {
							JPH::MeshShapeSettings meshSettings(triangles);
							shape = meshSettings.Create().Get();
						}
						else {
							GetPhysics().log->warn("Failed to extract mesh data, falling back to box collider");
							shape     = new JPH::BoxShape(shapeSize);
							shapeType = "Box";
						}
					}
					else {
						GetPhysics().log->warn("ModelRenderer has no model loaded, falling back to box collider");
						shape     = new JPH::BoxShape(shapeSize);
						shapeType = "Box";
					}
				}
				else {
					// default to box
					GetPhysics().log->warn("Invalid model, falling back to box collider");
					shape     = new JPH::BoxShape(shapeSize);
					shapeType = "Box";
				}
			}

			RVec3 startPos(0, 0, 0);
			Quat  startRot = Quat::sIdentity();

			if (entity.HasComponent<Transform>()) {
				auto&     tr  = entity.GetComponent<Transform>();
				glm::vec3 pos = tr.GetWorldPosition();
				glm::quat qt  = tr.GetWorldRotation();
				startPos      = Vec3(pos.x, pos.y, pos.z);
				startRot      = ToJolt(qt);
			}

			JPH::BodyCreationSettings settings(shape, startPos, startRot, (JPH::EMotionType) motionType, Layers::MOVING);

			auto                physics       = GetPhysics().GetPhysicsSystem();
			JPH::BodyInterface& bodyInterface = physics->GetBodyInterface();

			settings.mMassPropertiesOverride.mMass = mass;
			settings.mFriction                     = friction;
			settings.mRestitution                  = restitution;
			settings.mOverrideMassProperties       = EOverrideMassProperties::MassAndInertiaProvided;

			GetPhysics().log->info("Creating body with shape type {}", shapeType);

			bodyID = bodyInterface.CreateAndAddBody(settings, JPH::EActivation::Activate);
		}
		GetPhysics().GetPhysicsSystem()->GetBodyInterface().SetMotionQuality(bodyID, EMotionQuality::Discrete);
		GetPhysics().bodyToEntityMap[bodyID] = entity;
	}

	const char* items[] = {"Box", "Sphere", "Capsule", "Cylinder", "Mesh", "ConvexMesh"};


	void RigidBodyComponent::RenderInspector(Entity& entity)
	{
		ImGui::Text("Body ID: %u", bodyID.GetIndex());
		int shape_index = 0;

		auto                physics       = GetPhysics().GetPhysicsSystem();
		JPH::BodyInterface& bodyInterface = physics->GetBodyInterface();


		const char* motionTypes[] = {"Kinematic", "Dynamic"};
		int         current       = (motionType == (int) JPH::EMotionType::Kinematic) ? 0 : 1;

		if (LeftLabelCombo("Motion Type", &current, motionTypes, IM_ARRAYSIZE(motionTypes))) {
			// Update when changed
			motionType = (current == 0) ? (int) JPH::EMotionType::Kinematic : (int) JPH::EMotionType::Dynamic;

			bodyInterface.SetMotionType(bodyID, (JPH::EMotionType) motionType, JPH::EActivation::Activate);
		}

		if (LeftLabelSliderFloat("Mass", &mass, 1.0, 1000.0)) {
			JPH::BodyLockWrite lock(physics->GetBodyLockInterface(), bodyID);
			if (lock.Succeeded()) {
				auto&               body = lock.GetBody();
				auto*               mot  = body.GetMotionProperties();
				JPH::MassProperties mp;
				mp.ScaleToMass(mass);
				mot->SetMassProperties(JPH::EAllowedDOFs::All, mp);
			}
		}

		if (LeftLabelSliderFloat("Friction", &friction, 0.00, 1.0)) {
			JPH::BodyLockWrite lock(physics->GetBodyLockInterface(), bodyID);
			if (lock.Succeeded()) {
				auto& body = lock.GetBody();
				body.SetFriction(friction);
			}
		}

		if (LeftLabelSliderFloat("Restitution", &restitution, 0.00, 1.0)) {
			JPH::BodyLockWrite lock(physics->GetBodyLockInterface(), bodyID);
			if (lock.Succeeded()) {
				auto& body = lock.GetBody();
				body.SetRestitution(restitution);
			}
		}

		if (LeftLabelSliderFloat("Gravity Factor", &gravityFactor, 0.00, 2.0)) {
			bodyInterface.SetGravityFactor(bodyID, gravityFactor);
		}


		if (entity.HasComponent<Transform>()) {
			auto& tr = entity.GetComponent<Transform>();

			if (ImGui::TreeNode("Collider Settings")) {
				// Get currently selected option
				if (shapeType == "Box") {
					shape_index = 0;
				}
				else if (shapeType == "Sphere") {
					shape_index = 1;
				}
				else if (shapeType == "Capsule") {
					shape_index = 2;
				}
				else if (shapeType == "Cylinder") {
					shape_index = 3;
				}
				else if (shapeType == "Mesh") {
					shape_index = 4;
				}
				else if (shapeType == "ConvexMesh") {
					shape_index = 5;
				}


				if (LeftLabelBeginCombo("Shape", items[shape_index])) // Label + preview
				{
					for (int n = 0; n < IM_ARRAYSIZE(items); n++) {
						bool is_selected = (shape_index == n);
						if (ImGui::Selectable(items[n], is_selected)) {
							if (is_selected) {
								ImGui::SetItemDefaultFocus();
								continue;
							}

							if (n == 0) {
								Vec3                 size   = Vec3(tr.GetWorldScale().x / 2.0f, tr.GetWorldScale().y / 2.0f, tr.GetWorldScale().z / 2.0f);
								JPH::Ref<JPH::Shape> newBox = new JPH::BoxShape(size);
								bodyInterface.SetShape(bodyID, newBox, true, JPH::EActivation::Activate);
								shapeType = "Box";
								shapeSize = size;
							}
							else if (n == 1) {
								Vec3 size = Vec3(tr.GetWorldScale().x / 2.0f, 0.0f, 0.0f);

								JPH::Ref<JPH::Shape> newSphere = new JPH::SphereShape(size.GetX());
								bodyInterface.SetShape(bodyID, newSphere, true, JPH::EActivation::Activate);
								shapeType = "Sphere";
								shapeSize = size;
							}
							else if (n == 2) {
								Vec3 size = Vec3(tr.GetWorldScale().y / 4.0f, tr.GetWorldScale().x / 2.0f, 0.0f);

								JPH::Ref<JPH::Shape> newCapsule = new JPH::CapsuleShape(size.GetX(), size.GetY());
								bodyInterface.SetShape(bodyID, newCapsule, true, JPH::EActivation::Activate);
								shapeType = "Capsule";
								shapeSize = size;
							}
							else if (n == 3) {
								Vec3 size = Vec3(tr.GetWorldScale().y / 4.0f, tr.GetWorldScale().x / 2.0f, 0.0f);

								JPH::Ref<JPH::Shape> newCylinder = new JPH::CylinderShape(size.GetX(), size.GetY());
								bodyInterface.SetShape(bodyID, newCylinder, true, JPH::EActivation::Activate);
								shapeType = "Cylinder";
								shapeSize = size;
							}
							else if (n == 4) {
								// Create mesh collider from ModelRenderer
								SetMeshShape(entity);
							}
							else if (n == 5) {
								// Create convex mesh collider from ModelRenderer
								SetConvexMeshShape(entity);
							}
						}
					}
					LeftLabelEndCombo();
				}

				auto shape = bodyInterface.GetShape(bodyID);

				if (shapeType == "Box") {
					const auto* box_shape = static_cast<const BoxShape*>(shape.GetPtr());

					float halfExtents[3] = {box_shape->GetHalfExtent().GetX(), box_shape->GetHalfExtent().GetY(), box_shape->GetHalfExtent().GetZ()};

					if (LeftLabelDragFloat3("Half Extents", halfExtents, 0.25f)) {
						Vec3                 size   = Vec3(halfExtents[0], halfExtents[1], halfExtents[2]);
						JPH::Ref<JPH::Shape> newBox = new JPH::BoxShape(size);
						bodyInterface.SetShape(bodyID, newBox, true, JPH::EActivation::Activate);
						shapeSize = size;
					}
				}
				else if (shapeType == "Sphere") {
					const auto* sphere_shape = static_cast<const SphereShape*>(shape.GetPtr());

					float radius = sphere_shape->GetRadius();
					if (LeftLabelDragFloat("Radius", &radius, 0.25f)) {
						JPH::Ref<JPH::Shape> newSphere = new JPH::SphereShape(radius);
						bodyInterface.SetShape(bodyID, newSphere, true, JPH::EActivation::Activate);

						shapeSize = Vec3(radius, 0.0, 0.0);
					}
				}
				else if (shapeType == "Cylinder") {
					const auto* cylinder_shape = static_cast<const CylinderShape*>(shape.GetPtr());


					float radius     = cylinder_shape->GetRadius();
					float halfHeight = cylinder_shape->GetHalfHeight();

					if (LeftLabelDragFloat("Radius", &radius, 0.25f) || LeftLabelDragFloat("Half Height", &halfHeight, 0.25f)) {
						JPH::Ref<JPH::Shape> newCylinder = new JPH::CylinderShape(halfHeight, radius);
						bodyInterface.SetShape(bodyID, newCylinder, true, JPH::EActivation::Activate);

						shapeSize = Vec3(halfHeight, radius, 0.0);
					}
				}
				else if (shapeType == "Capsule") {
					const auto* capsule_shape = static_cast<const CapsuleShape*>(shape.GetPtr());

					float radius     = capsule_shape->GetRadius();
					float halfHeight = capsule_shape->GetHalfHeightOfCylinder();

					if (LeftLabelDragFloat("Radius", &radius, 0.25f) || LeftLabelDragFloat("Half Height", &halfHeight, 0.25f)) {
						JPH::Ref<JPH::Shape> newCapsule = new JPH::CapsuleShape(halfHeight, radius);
						bodyInterface.SetShape(bodyID, newCapsule, true, JPH::EActivation::Activate);

						shapeSize = Vec3(halfHeight, radius, 0.0);
					}
				}
				else if (shapeType == "Mesh") {
					// Display read-only mesh info
					ImGui::TextWrapped("Mesh Collider (from ModelRenderer)");
					ImGui::TextWrapped("Mesh colliders are read-only and derive from the entity's model geometry.");

					if (LeftLabelAssetModel("Collider Model", &colliderModel)) {
						SetMeshShape(entity);
					}

					if (ImGui::Button("Refresh from Model")) {
						SetMeshShape(entity);
					}

					if (colliderModel.IsValid()) {
						auto* model = GetAssetManager().Get(colliderModel);
						if (model) {
							const auto& meshes = model->GetMeshes();
							if (meshSelection.size() != meshes.size()) {
								meshSelection.resize(meshes.size(), true);
							}

							if (ImGui::TreeNode("Mesh Selection")) {
								bool changed = false;
								for (size_t i = 0; i < meshes.size(); ++i) {
									std::string label = "Mesh " + std::to_string(i);
									// Use bool for ImGui
									bool enabled = meshSelection[i];
									if (ImGui::Checkbox(label.c_str(), &enabled)) {
										meshSelection[i] = enabled;
										changed          = true;
									}
								}
								ImGui::TreePop();

								if (changed) {
									SetMeshShape(entity);
								}
							}
						}
					}
				}
				else if (shapeType == "ConvexMesh") {
					// Display convex mesh info
					ImGui::TextWrapped("Convex Mesh Collider (from ModelRenderer)");
					ImGui::TextWrapped("Convex mesh colliders are suitable for dynamic bodies and support proper mass/inertia calculation.");

					if (LeftLabelAssetModel("Collider Model", &colliderModel)) {
						SetConvexMeshShape(entity);
					}

					if (ImGui::Button("Refresh from Model##ConvexMesh")) {
						SetConvexMeshShape(entity);
					}

					if (colliderModel.IsValid()) {
						auto* model = GetAssetManager().Get(colliderModel);
						if (model) {
							const auto& meshes = model->GetMeshes();
							if (meshSelection.size() != meshes.size()) {
								meshSelection.resize(meshes.size(), true);
							}

							if (ImGui::TreeNode("Mesh Selection##ConvexMesh")) {
								bool changed = false;
								for (size_t i = 0; i < meshes.size(); ++i) {
									std::string label   = "Mesh " + std::to_string(i) + "##Convex";
									bool        enabled = meshSelection[i];
									if (ImGui::Checkbox(label.c_str(), &enabled)) {
										meshSelection[i] = enabled;
										changed          = true;
									}
								}
								ImGui::TreePop();

								if (changed) {
									SetConvexMeshShape(entity);
								}
							}
						}
					}
				}
				ImGui::TreePop();
			}
		}
	}


	void RigidBodyComponent::AddBindings()
	{
		auto& lua = GetScriptManager().lua;


		lua.new_usertype<RigidBodyComponent>("RigidBodyComponent",
		                                     "getPosition",
		                                     &RigidBodyComponent::GetPosition,
		                                     "setPosition",
		                                     &RigidBodyComponent::SetPosition,
		                                     "setRotation",
		                                     &RigidBodyComponent::SetRotation,
		                                     "setRotationEuler",
		                                     &RigidBodyComponent::SetRotationEuler,
		                                     "getLinearVelocity",
		                                     &RigidBodyComponent::GetLinearVelocity,
		                                     "addLinearVelocity",
		                                     &RigidBodyComponent::AddLinearVelocity,
		                                     "setLinearVelocity",
		                                     &RigidBodyComponent::SetLinearVelocity,
		                                     "getAngularVelocity",
		                                     &RigidBodyComponent::GetAngularVelocity,
		                                     "setAngularVelocity",
		                                     &RigidBodyComponent::SetAngularVelocity,
		                                     "applyForce",
		                                     &RigidBodyComponent::ApplyForce,
		                                     "applyImpulse",
		                                     &RigidBodyComponent::ApplyImpulse,
		                                     "applyTorque",
		                                     &RigidBodyComponent::ApplyTorque,
		                                     "applyAngularImpulse",
		                                     &RigidBodyComponent::ApplyAngularImpulse,
		                                     "setGravityFactor",
		                                     &RigidBodyComponent::SetGravityFactor,
		                                     "getGravityFactor",
		                                     &RigidBodyComponent::GetGravityFactor,
		                                     "activate",
		                                     &RigidBodyComponent::Activate,
		                                     "deactivate",
		                                     &RigidBodyComponent::Deactivate,
		                                     "isActive",
		                                     &RigidBodyComponent::IsActive,
		                                     "setKinematic",
		                                     &RigidBodyComponent::SetKinematic,
		                                     "isKinematic",
		                                     &RigidBodyComponent::IsKinematic,


		                                     "moveKinematic",
		                                     &RigidBodyComponent::MoveKinematic,


		                                     "setCollisionShape",
		                                     &RigidBodyComponent::SetCollisionShape,
		                                     "setCollisionShapeRef",
		                                     &RigidBodyComponent::SetCollisionShapeRef,

		                                     "setBoxShape",
		                                     &RigidBodyComponent::SetBoxShape,
		                                     "setSphereShape",
		                                     &RigidBodyComponent::SetSphereShape,
		                                     "setCapsuleShape",
		                                     &RigidBodyComponent::SetCapsuleShape,
		                                     "setCylinderShape",
		                                     &RigidBodyComponent::SetCylinderShape,
		                                     "setMeshShape",
		                                     &RigidBodyComponent::SetMeshShape,
		                                     "setConvexMeshShape",
		                                     &RigidBodyComponent::SetConvexMeshShape);
	}


	// === Conversion Utilities ===
	JPH::Vec3 RigidBodyComponent::ToJolt(const glm::vec3& v)
	{
		return {v.x, v.y, v.z};
	}

	glm::vec3 RigidBodyComponent::ToGlm(const JPH::Vec3& v)
	{
		return {v.GetX(), v.GetY(), v.GetZ()};
	}

	JPH::Quat RigidBodyComponent::ToJolt(const glm::quat& q)
	{
		glm::quat normalized = glm::normalize(q);
		return {normalized.x, normalized.y, normalized.z, normalized.w};
	}

	glm::quat RigidBodyComponent::ToGlm(const JPH::Quat& q)
	{
		return {q.GetW(), q.GetX(), q.GetY(), q.GetZ()}; // glm uses w first
	}

	// === API ===

	void RigidBodyComponent::SetCollisionShape(const JPH::ShapeSettings& settings)
	{
		GetPhysics().log->debug("SETT SetCollisionShape");
		JPH::Shape::ShapeResult result = settings.Create();
		if (result.HasError()) {
			GetPhysics().log->error("Shape creation failed: " + result.GetError());
		}
		SetCollisionShapeRef(result.Get());
	}

	void RigidBodyComponent::SetCollisionShapeRef(const JPH::ShapeRefC& shape)
	{
		auto& bodyInterface = GetPhysics().GetPhysicsSystem()->GetBodyInterface();
		// Mesh shapes cannot calculate mass/inertia, so we must not update mass properties automatically
		bool updateMass = shape.GetPtr()->GetSubType() != EShapeSubType::Mesh;
		bodyInterface.SetShape(bodyID, shape, updateMass, JPH::EActivation::Activate);

		if (shape.GetPtr()->GetSubType() == EShapeSubType::Box) {
			shapeType             = "Box";
			const auto* box_shape = static_cast<const BoxShape*>(shape.GetPtr());
			shapeSize             = box_shape->GetHalfExtent();
		}
		else if (shape.GetPtr()->GetSubType() == EShapeSubType::Sphere) {
			shapeType                = "Sphere";
			const auto* sphere_shape = static_cast<const SphereShape*>(shape.GetPtr());
			shapeSize                = Vec3(sphere_shape->GetRadius(), 0.0, 0.0);
		}
		else if (shape.GetPtr()->GetSubType() == EShapeSubType::Cylinder) {
			shapeType                  = "Cylinder";
			const auto* cylinder_shape = static_cast<const CylinderShape*>(shape.GetPtr());
			shapeSize                  = Vec3(cylinder_shape->GetHalfHeight(), cylinder_shape->GetRadius(), 0.0);
		}
		else if (shape.GetPtr()->GetSubType() == EShapeSubType::Capsule) {
			shapeType                 = "Capsule";
			const auto* capsule_shape = static_cast<const CapsuleShape*>(shape.GetPtr());
			shapeSize                 = Vec3(capsule_shape->GetHalfHeightOfCylinder(), capsule_shape->GetRadius(), 0.0);
		}
		else if (shape.GetPtr()->GetSubType() == EShapeSubType::Mesh) {
			shapeType = "Mesh";
			// Mesh shape size is not stored since it's derived from the model
		}
		else if (shape.GetPtr()->GetSubType() == EShapeSubType::ConvexHull) {
			shapeType = "ConvexMesh";
			// Convex mesh shape size is not stored since it's derived from the model
		}
	}


	void RigidBodyComponent::SetBoxShape(const JPH::BoxShapeSettings& settings)
	{
		auto result = settings.Create();
		if (result.HasError()) {
			GetPhysics().log->error("BoxShape creation failed: " + result.GetError());
		}
		SetCollisionShapeRef(result.Get());
		shapeType = "Box";
		shapeSize = settings.mHalfExtent;
	}

	void RigidBodyComponent::SetSphereShape(const JPH::SphereShapeSettings& settings)
	{
		auto result = settings.Create();
		if (result.HasError()) {
			GetPhysics().log->error("SphereShape creation failed: " + result.GetError());
		}
		SetCollisionShapeRef(result.Get());
		shapeType = "Sphere";
		shapeSize = Vec3(settings.mRadius, 0.0, 0.0);
	}

	void RigidBodyComponent::SetCapsuleShape(const JPH::CapsuleShapeSettings& settings)
	{
		auto result = settings.Create();
		if (result.HasError()) {
			GetPhysics().log->error("CapsuleShape creation failed: " + result.GetError());
		}
		SetCollisionShapeRef(result.Get());
		shapeType = "Capsule";
		shapeSize = Vec3(settings.mHalfHeightOfCylinder, settings.mRadius, 0.0);
	}

	void RigidBodyComponent::SetCylinderShape(const JPH::CylinderShapeSettings& settings)
	{
		auto result = settings.Create();
		if (result.HasError()) {
			GetPhysics().log->error("CylinderShape creation failed: " + result.GetError());
		}
		SetCollisionShapeRef(result.Get());
		shapeType = "Cylinder";
		shapeSize = Vec3(settings.mHalfHeight, settings.mRadius, 0.0);
	}

	void RigidBodyComponent::SetMeshShape(Entity& entity)
	{
		shapeType = "Mesh";

		// Check if colliderModel is set
		if (!colliderModel.IsValid()) {
			GetPhysics().log->warn("Cannot create mesh collider: No collider model set");
			return;
		}

		auto* model = GetAssetManager().Get(colliderModel);
		if (!model) {
			GetPhysics().log->warn("Cannot create mesh collider: Collider model could not be loaded");
			return;
		}

		JPH::TriangleList triangles;
		const auto&       meshes = model->GetMeshes();

		// Ensure mesh selection matches model
		if (meshSelection.size() != meshes.size()) {
			meshSelection.resize(meshes.size(), true);
		}

		// Extract triangle data from all meshes
		int meshIndex = 0;
		for (const auto& mesh : meshes) {
			// Skip disabled meshes
			if (!meshSelection[meshIndex]) {
				meshIndex++;
				continue;
			}
			const auto& vertices = mesh->GetVertices();
			const auto& indices  = mesh->GetIndices();

			// Convert indices to triangles
			for (size_t i = 0; i < indices.size(); i += 3) {
				if (i + 2 < indices.size()) {
					const auto& v0 = vertices[indices[i]].Position;
					const auto& v1 = vertices[indices[i + 1]].Position;
					const auto& v2 = vertices[indices[i + 2]].Position;

					JPH::Triangle triangle(JPH::Float3(v0.x, v0.y, v0.z), JPH::Float3(v1.x, v1.y, v1.z), JPH::Float3(v2.x, v2.y, v2.z));
					triangles.push_back(triangle);
				}
			}
			meshIndex++;
		}

		if (triangles.empty()) {
			GetPhysics().log->warn("Cannot create mesh collider: No valid triangles found in model");
			return;
		}

		// Create mesh shape
		JPH::MeshShapeSettings meshSettings(triangles);
		auto                   result = meshSettings.Create();
		if (result.HasError()) {
			GetPhysics().log->error("MeshShape creation failed: " + result.GetError());
			return;
		}

		SetCollisionShapeRef(result.Get());
		shapeType = "Mesh";
	}

	void RigidBodyComponent::SetConvexMeshShape(Entity& entity)
	{
		shapeType = "ConvexMesh";

		// Check if colliderModel is set
		if (!colliderModel.IsValid()) {
			GetPhysics().log->warn("Cannot create convex mesh collider: No collider model set");
			return;
		}

		auto* model = GetAssetManager().Get(colliderModel);
		if (!model) {
			GetPhysics().log->warn("Cannot create convex mesh collider: Collider model could not be loaded");
			return;
		}

		JPH::Array<JPH::Vec3> points;
		const auto&           meshes = model->GetMeshes();

		// Ensure mesh selection matches model
		if (meshSelection.size() != meshes.size()) {
			GetPhysics().log->warn("Mesh selection size does not match model size, resizing to match. Was {} now {}", meshSelection.size(), meshes.size());
			meshSelection.resize(meshes.size(), true);
		}

		// Extract vertex positions from all selected meshes
		int meshIndex = 0;
		for (const auto& mesh : meshes) {
			// Skip disabled meshes
			if (!meshSelection[meshIndex]) {
				meshIndex++;
				continue;
			}
			const auto& vertices = mesh->GetVertices();

			// Add all vertices as points for convex hull
			for (const auto& vertex : vertices) {
				points.push_back(JPH::Vec3(vertex.Position.x, vertex.Position.y, vertex.Position.z));
			}
			meshIndex++;
		}

		if (points.empty()) {
			GetPhysics().log->warn("Cannot create convex mesh collider: No valid vertices found in model");
			return;
		}

		// Create convex hull shape
		JPH::ConvexHullShapeSettings convexSettings(points);
		auto                         result = convexSettings.Create();
		if (result.HasError()) {
			GetPhysics().log->error("ConvexHullShape creation failed: " + result.GetError());
			return;
		}

		// Store the center of mass offset for later use in physics sync
		centerOfMassOffset = result.Get()->GetCenterOfMass();

		SetCollisionShapeRef(result.Get());
		shapeType = "ConvexMesh";
	}


	glm::vec3 RigidBodyComponent::GetPosition() const
	{
		return ToGlm(GetPhysics().GetPhysicsSystem()->GetBodyInterface().GetPosition(bodyID));
	}

	void RigidBodyComponent::MoveKinematic(const glm::vec3& position, const glm::quat& rotation, float dt)
	{
		GetPhysics().GetPhysicsSystem()->GetBodyInterface().MoveKinematic(bodyID, ToJolt(position), ToJolt(rotation), dt);
	}
	void RigidBodyComponent::SetPosition(const glm::vec3& position)
	{
		GetPhysics().GetPhysicsSystem()->GetBodyInterface().SetPosition(bodyID, ToJolt(position), JPH::EActivation::Activate);
	}

	void RigidBodyComponent::SetRotation(const glm::quat& rotation)
	{
		GetPhysics().GetPhysicsSystem()->GetBodyInterface().SetRotation(bodyID, ToJolt(rotation), JPH::EActivation::Activate);
	}

	void RigidBodyComponent::SetRotationEuler(const glm::vec3& eulerAngles)
	{
		GetPhysics().GetPhysicsSystem()->GetBodyInterface().SetRotation(bodyID, ToJolt(glm::quat(glm::radians(eulerAngles))), JPH::EActivation::Activate);
	}

	void RigidBodyComponent::SetLinearVelocity(const glm::vec3& velocity)
	{
		GetPhysics().GetPhysicsSystem()->GetBodyInterface().SetLinearVelocity(bodyID, ToJolt(velocity));
	}

	void RigidBodyComponent::AddLinearVelocity(const glm::vec3& velocity)
	{
		GetPhysics().GetPhysicsSystem()->GetBodyInterface().AddLinearVelocity(bodyID, ToJolt(velocity));
	}

	glm::vec3 RigidBodyComponent::GetLinearVelocity() const
	{
		return ToGlm(GetPhysics().GetPhysicsSystem()->GetBodyInterface().GetLinearVelocity(bodyID));
	}

	void RigidBodyComponent::SetAngularVelocity(const glm::vec3& velocity)
	{
		GetPhysics().GetPhysicsSystem()->GetBodyInterface().SetAngularVelocity(bodyID, ToJolt(velocity));
	}

	glm::vec3 RigidBodyComponent::GetAngularVelocity() const
	{
		return ToGlm(GetPhysics().GetPhysicsSystem()->GetBodyInterface().GetAngularVelocity(bodyID));
	}

	void RigidBodyComponent::ApplyForce(const glm::vec3& force)
	{
		GetPhysics().GetPhysicsSystem()->GetBodyInterface().AddForce(bodyID, ToJolt(force));
	}

	void RigidBodyComponent::ApplyImpulse(const glm::vec3& impulse)
	{
		GetPhysics().GetPhysicsSystem()->GetBodyInterface().AddImpulse(bodyID, ToJolt(impulse));
	}

	void RigidBodyComponent::ApplyTorque(const glm::vec3& torque)
	{
		GetPhysics().GetPhysicsSystem()->GetBodyInterface().AddTorque(bodyID, ToJolt(torque));
	}

	void RigidBodyComponent::ApplyAngularImpulse(const glm::vec3& impulse)
	{
		GetPhysics().GetPhysicsSystem()->GetBodyInterface().AddAngularImpulse(bodyID, ToJolt(impulse));
	}

	void RigidBodyComponent::SetGravityFactor(float factor)
	{
		GetPhysics().GetPhysicsSystem()->GetBodyInterface().SetGravityFactor(bodyID, factor);
		gravityFactor = factor;
	}

	float RigidBodyComponent::GetGravityFactor() const
	{
		return GetPhysics().GetPhysicsSystem()->GetBodyInterface().GetGravityFactor(bodyID);
	}

	void RigidBodyComponent::Activate()
	{
		GetPhysics().GetPhysicsSystem()->GetBodyInterface().ActivateBody(bodyID);
	}

	void RigidBodyComponent::Deactivate()
	{
		GetPhysics().GetPhysicsSystem()->GetBodyInterface().DeactivateBody(bodyID);
	}

	bool RigidBodyComponent::IsActive() const
	{
		return GetPhysics().GetPhysicsSystem()->GetBodyInterface().IsActive(bodyID);
	}

	void RigidBodyComponent::SetKinematic(bool enable)
	{
		GetPhysics().GetPhysicsSystem()->GetBodyInterface().SetMotionType(bodyID, enable ? JPH::EMotionType::Kinematic : JPH::EMotionType::Dynamic, JPH::EActivation::DontActivate);
		motionType = enable ? (int) JPH::EMotionType::Kinematic : (int) JPH::EMotionType::Dynamic;
	}

	bool RigidBodyComponent::IsKinematic() const
	{
		return GetPhysics().GetPhysicsSystem()->GetBodyInterface().GetMotionType(bodyID) == JPH::EMotionType::Kinematic;
	}

	RigidBodyComponent::RigidBodyComponent(const RigidBodyComponent& other)
	{
		motionType    = other.motionType;
		mass          = other.mass;
		friction      = other.friction;
		restitution   = other.restitution;
		gravityFactor = other.gravityFactor;
		shapeType     = other.shapeType;
		shapeSize     = other.shapeSize;
		meshSelection = other.meshSelection;
		colliderModel = other.colliderModel;
		// prevent all elements of meshSelection to be false
		if (meshSelection.size() > 0 && std::all_of(meshSelection.begin(), meshSelection.end(), [](bool b) { return !b; })) {
			meshSelection[0] = true;
		}
	}


} // namespace Engine::Components