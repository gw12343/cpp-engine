//
// Created by gabe on 8/14/25.
//

#ifndef CPP_ENGINE_ALLCOMPONENTS_H
#define CPP_ENGINE_ALLCOMPONENTS_H

#include "impl/AnimationComponent.h"
#include "impl/AnimationPoseComponent.h"
#include "impl/AnimationWorkerComponent.h"
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

#define COMPONENT_LIST                                                                                                                                                                                                                         \
	X(Components::LuaScript, LuaScript)                                                                                                                                                                                                        \
	X(Components::ShadowCaster, ShadowCaster)                                                                                                                                                                                                  \
	X(Components::Transform, Transform)                                                                                                                                                                                                        \
	X(Components::ModelRenderer, ModelRenderer)                                                                                                                                                                                                \
	X(Components::RigidBodyComponent, RigidBodyComponent)                                                                                                                                                                                      \
	X(Components::AudioSource, AudioSource)                                                                                                                                                                                                    \
	X(Components::SkeletonComponent, SkeletonComponent)                                                                                                                                                                                        \
	X(Components::AnimationComponent, AnimationComponent)                                                                                                                                                                                      \
	X(Components::AnimationPoseComponent, AnimationPoseComponent)                                                                                                                                                                              \
	X(Components::AnimationWorkerComponent, AnimationWorkerComponent)                                                                                                                                                                          \
	X(Components::SkinnedMeshComponent, SkinnedMeshComponent)                                                                                                                                                                                  \
	X(Components::ParticleSystem, ParticleSystem)


#endif // CPP_ENGINE_ALLCOMPONENTS_H
