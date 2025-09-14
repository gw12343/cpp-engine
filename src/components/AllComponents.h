//
// Created by gabe on 8/14/25.
//

#ifndef CPP_ENGINE_ALLCOMPONENTS_H
#define CPP_ENGINE_ALLCOMPONENTS_H

#include "impl/AnimationComponent.h"
#include "impl/AnimationPoseComponent.h"
#include "impl/AudioSourceComponent.h"
#include "impl/EntityMetadataComponent.h"
#include "impl/LuaScriptComponent.h"
#include "impl/ModelRendererComponent.h"
#include "impl/ParticleSystemComponent.h"
#include "impl/RigidBodyComponent.h"
#include "impl/ShadowCasterComponent.h"
#include "impl/SkeletonComponent.h"
#include "impl/SkinnedMeshComponent.h"
#include "impl/TerrainRendererComponent.h"
#include "impl/TransformComponent.h"
#include "impl/PlayerControllerComponent.h"

#define COMPONENT_LIST                                                                                                                                                                                                                         \
	X(Components::LuaScript, LuaScript, ICON_FA_SCROLL " Script")                                                                                                                                                                              \
	X(Components::ShadowCaster, ShadowCaster, ICON_FA_MOON " Shadow Caster")                                                                                                                                                                   \
	X(Components::Transform, Transform, ICON_FA_MAXIMIZE " Transform")                                                                                                                                                                         \
	X(Components::TerrainRenderer, TerrainRenderer, ICON_FA_MAP " Terrain Renderer")                                                                                                                                                           \
	X(Components::ModelRenderer, ModelRenderer, ICON_FA_CUBE " Model Renderer")                                                                                                                                                                \
	X(Components::RigidBodyComponent, RigidBodyComponent, ICON_FA_CUBES_STACKED " Rigid Body")                                                                                                                                                 \
	X(Components::AudioSource, AudioSource, ICON_FA_VOLUME_HIGH " Audio Source")                                                                                                                                                               \
	X(Components::SkeletonComponent, SkeletonComponent, "Skeleton")                                                                                                                                                                            \
	X(Components::AnimationComponent, AnimationComponent, "Animation")                                                                                                                                                                         \
	X(Components::AnimationPoseComponent, AnimationPoseComponent, "Animation Pose")                                                                                                                                                            \
	X(Components::SkinnedMeshComponent, SkinnedMeshComponent, "Skinned Mesh")                                                                                                                                                                  \
	X(Components::ParticleSystem, ParticleSystem, ICON_FA_STAR_HALF_STROKE "Particle System")                                                                                                                                                  \
	X(Components::PlayerControllerComponent, PlayerControllerComponent, "Player Controller")

#endif // CPP_ENGINE_ALLCOMPONENTS_H
