//
// Created by gabe on 6/24/25.
//
#include "ParticleSystemComponent.h"
#include "TransformComponent.h"

#include "core/Engine.h"
#include "core/Entity.h"
#include "core/EngineData.h"



#include "ozz/animation/runtime/track.h"
#include "rendering/particles/ParticleManager.h"
#include "animation/AnimationManager.h"
#include "scripting/ScriptManager.h"

#include "misc/cpp/imgui_stdlib.h"



namespace Engine::Components {


    void ParticleSystem::AddBindings()
    {
        auto& lua = GetScriptManager().lua;

        lua.new_usertype<ParticleSystem>("ParticleSystem",
                                         "autoPlay",
                                         &ParticleSystem::autoPlay,
                                         "looping",
                                         &ParticleSystem::looping,
                                         "play",
                                         [](ParticleSystem& self, Entity entity) {
	                                         // Prefer calling through the manager so handles stay consistent.
	                                         GetParticleManager().PlayEffect(entity);
                                         });
    }

	void ParticleSystem::OnRemoved(Entity& entity)
	{
		auto manager = GetParticleManager().GetManager();
		if (manager && handle >= 0) {
			manager->StopRoot(handle); // force stop this one instance
			handle = -1;
		}

		// release effect reference
	}


	void ParticleSystem::OnAdded(Entity& entity)
	{

	}

	void ParticleSystem::RenderInspector(Entity& entity)
	{
		ImGui::Text("Handle: %d", handle);
		const auto& manager = GetParticleManager().GetManager();
		ENGINE_VERIFY(manager != nullptr, "ParticleSystem::RenderInspector: Failed to get Effekseer manager");
		bool paused = manager->GetPaused(handle);

        LeftLabelAssetParticle("Particle Effect", &effect);


        LeftLabelCheckbox("Autoplay", &autoPlay);
        LeftLabelCheckbox("Loop", &looping);

		if (ImGui::Button(paused ? "Unpause" : "Pause")) {
			manager->SetPaused(handle, !paused);
		}

		if (ImGui::Button("Restart")) {
			manager->StopEffect(handle);
			auto      transform = entity.GetComponent<Components::Transform>();
			auto      pos       = transform.GetWorldPosition();
			Particle* particle  = GetAssetManager().Get(effect);
			handle              = manager->Play(particle->GetEffect(), pos.x, pos.y, pos.z);
		}
	}


} // namespace Engine::Components

#include "assets/AssetManager.inl"