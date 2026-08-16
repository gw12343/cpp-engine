//
// Created by Gabe on 6/8/2026.
//

#include "ComponentMethodBinder.h"

#include <sol/state.hpp>

#include "core/Entity.h"
#include "components/impl/AnimationComponent.h"
#include "assets/impl/ModelLoader.h"
#include "components/impl/LuaScriptComponent.h"
#include "components/impl/ShadowCasterComponent.h"
#include "components/impl/EntityMetadataComponent.h"
#include "components/impl/ModelRendererComponent.h"
#include "components/impl/RigidBodyComponent.h"
#include "components/impl/AudioSourceComponent.h"
#include "components/impl/SkinnedMeshComponent.h"
#include "components/impl/ParticleSystemComponent.h"
#include "components/AllComponents.h"
#include "assets/Prefab.h"

namespace Engine {


#define COMPONENT_METHODS(COMPONENT_TYPE, COMPONENT_NAME)                                                                                                                                                                                      \
"Add" #COMPONENT_NAME, [](Entity& e) -> COMPONENT_TYPE& { return e.AddComponent<COMPONENT_TYPE>(); }, "Get" #COMPONENT_NAME, [](Entity& e) -> COMPONENT_TYPE& { return e.GetComponent<COMPONENT_TYPE>(); }, "Has" #COMPONENT_NAME,         \
[](Entity& e) -> bool { return e.HasComponent<COMPONENT_TYPE>(); }, "Remove" #COMPONENT_NAME, [](Entity& e) { e.RemoveComponent<COMPONENT_TYPE>(); }

    void ComponentMethodBinder::BindMethodsLua(sol::state* state) {
    #define X(type, name, fancy) COMPONENT_METHODS(type, name),
            // Bind Entity
            state->new_usertype<Entity>(
                "Entity",
                "isValid",
                [](const Entity& e) { return static_cast<bool>(e); },
                "getName",
                &Entity::GetName,
                "getTag",
                &Entity::GetTag,
                "setName",
                &Entity::SetName,
                "setParent",
                &Entity::SetParent,
                "getHandle",
                &Entity::GetEntityHandle,
                "getChildren",
                &Entity::GetChildren,
                "destroy",
                &Entity::MarkForDestruction,
                "instantiatePrefab",
                [](Entity& parent, const PrefabHandle& handle) { return InstantiatePrefab(handle, parent.GetEntityHandle()); },
                COMPONENT_LIST COMPONENT_METHODS(Components::EntityMetadata, EntityMetadata));
    #undef X
    }

}