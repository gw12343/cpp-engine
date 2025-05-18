#include "components/Components.h"
#include "core/Entity.h"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include "core/Engine.h"

namespace Engine {

namespace Components {

    void EntityMetadata::OnAdded(Entity& entity) {
    }

    void Transform::OnAdded(Entity& entity) {
    }

    void ModelRenderer::OnAdded(Entity& entity) {
    }

    void RigidBodyComponent::OnAdded(Entity& entity) {
    }

    void AudioSource::OnAdded(Entity& entity) {
        // If autoPlay is enabled, try to play the sound
        if (autoPlay && !soundName.empty()) {
            // Get the sound manager from the engine
            auto& soundManager = entity.m_engine->GetSoundManager();
            Play(soundManager);
        }
    }

    void EntityMetadata::RenderInspector(Entity& entity) {
        char nameBuffer[256];
        strcpy(nameBuffer, name.c_str());
        if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
            name = nameBuffer;
        }
        
        char tagBuffer[256];
        strcpy(tagBuffer, tag.c_str());
        if (ImGui::InputText("Tag", tagBuffer, sizeof(tagBuffer))) {
            tag = tagBuffer;
        }
        
        ImGui::Checkbox("Active", &active);
    }

    void Transform::RenderInspector(Entity& entity) {
        bool updatePhysicsPositionManually = false;
        if(ImGui::DragFloat3("Position", glm::value_ptr(position), 0.1f)) {
            updatePhysicsPositionManually = true;
        }
        
        glm::vec3 eulerAngles = GetEulerAngles();
        if (ImGui::DragFloat3("Rotation", glm::value_ptr(eulerAngles), 1.0f)) {
            SetRotation(eulerAngles);
            updatePhysicsPositionManually = true;
        }

        if(updatePhysicsPositionManually){
            if(entity.HasComponent<RigidBodyComponent>()) {
                auto& rb = entity.GetComponent<RigidBodyComponent>();
                BodyInterface& body_interface = rb.physicsSystem->GetBodyInterface();
                //convert pos and rot to jolt types
                RVec3 joltPos = RVec3(position.x, position.y, position.z);
                Quat joltRot = Quat::sEulerAngles(RVec3(glm::radians(eulerAngles.x), glm::radians(eulerAngles.y), glm::radians(eulerAngles.z)));
                body_interface.SetPositionAndRotation(rb.bodyID, joltPos, joltRot, EActivation::Activate);
            }
        }
        
        ImGui::DragFloat3("Scale", glm::value_ptr(scale), 0.1f);
    }

    void ModelRenderer::RenderInspector(Entity& entity) {
        ImGui::Checkbox("Visible", &visible);
        
        if (model) {
            ImGui::Text("Model: Loaded");
            // Could add more model info here
        } else {
            ImGui::Text("Model: None");
        }
    }

    void RigidBodyComponent::RenderInspector(Entity& entity) {
        ImGui::Text("Body ID: %u", bodyID.GetIndex());
        // Could add more physics properties here
    }

    void AudioSource::RenderInspector(Entity& entity) {
        char soundNameBuffer[256];
        strcpy(soundNameBuffer, soundName.c_str());
        if (ImGui::InputText("Sound", soundNameBuffer, sizeof(soundNameBuffer))) {
            soundName = soundNameBuffer;
        }
        
        ImGui::Checkbox("Auto Play", &autoPlay);
        ImGui::Checkbox("Looping", &looping);
        
        ImGui::SliderFloat("Volume", &volume, 0.0f, 1.0f);
        ImGui::SliderFloat("Pitch", &pitch, 0.5f, 2.0f);
        
        ImGui::Separator();
        ImGui::Text("Attenuation Settings:");
        ImGui::SliderFloat("Reference Distance", &referenceDistance, 0.1f, 20.0f);
        ImGui::SliderFloat("Max Distance", &maxDistance, 1.0f, 200.0f);
        ImGui::SliderFloat("Rolloff Factor", &rolloffFactor, 0.1f, 5.0f);
        
        ImGui::Separator();
        if (isPlaying) {
            if (ImGui::Button("Stop")) {
                Stop();
            }
        }
    }
}
}