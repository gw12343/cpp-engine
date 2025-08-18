//
// Created by gabe on 6/24/25.
//
#include "ParticleSystemComponent.h"
#include "TransformComponent.h"

#include "core/Engine.h"
#include "core/Entity.h"
#include "utils/Utils.h"

#include <codecvt>
#include <locale>

#include "imgui.h"
#include "ozz/animation/runtime/track.h"
#include <string>
#include "rendering/particles/ParticleManager.h"
#include "animation/AnimationManager.h"
#include "scripting/ScriptManager.h"
#include "assets/AssetManager.h"

namespace Engine::Components {
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
		SPDLOG_INFO("SYSTEM GUID: {}", effect.GetID());
		//		if (!effect) {
		//			// Convert path string
		//			ENGINE_ASSERT(!effectPath.empty(), "ParticleSystem::OnAdded: effectPath is empty");
		//			std::u16string  utf16 = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.from_bytes(effectPath);
		//			const char16_t* raw   = utf16.c_str();
		//
		//			const auto& manager = GetParticleManager().GetManager();
		//			ENGINE_VERIFY(manager != nullptr, "ParticleSystem::OnAdded: Failed to get Effekseer manager");
		//			// Spawn effect
		//			effect = Effekseer::Effect::Create(manager, raw);
		//			ENGINE_VERIFY(effect != nullptr, "ParticleSystem::OnAdded: Failed to create Effekseer effect");
		//			// Get initial transform
		//			auto transform = entity.GetComponent<Components::Transform>();
		//			// Spawn particle
		//			handle = manager->Play(effect, transform.position.x, transform.position.y, transform.position.z);
		//		}
		if (effect.IsValid()) {
			Particle* particle = GetAssetManager().Get(effect);
			ENGINE_VERIFY(particle, "Particle effect is null!");
			ENGINE_VERIFY(particle->IsValid(), "Particle effect incorrectly loaded!");
			// Get particle manager
			const auto& manager = GetParticleManager().GetManager();

			// Spawn particle system at transform
			auto transform = entity.GetComponent<Components::Transform>();
			handle         = manager->Play(particle->GetEffect(), transform.position.x, transform.position.y, transform.position.z);
		}
	}

	void ParticleSystem::RenderInspector(Entity& entity)
	{
		ImGui::Text("Handle: %d", handle);
		const auto& manager = GetParticleManager().GetManager();
		ENGINE_VERIFY(manager != nullptr, "ParticleSystem::RenderInspector: Failed to get Effekseer manager");
		bool paused = manager->GetPaused(handle);

		if (ImGui::Button(paused ? "Unpause" : "Pause")) {
			manager->SetPaused(handle, !paused);
		}

		if (ImGui::Button("Restart")) {
			manager->StopEffect(handle);
			auto      transform = entity.GetComponent<Components::Transform>();
			Particle* particle  = GetAssetManager().Get(effect);
			handle              = manager->Play(particle->GetEffect(), transform.position.x, transform.position.y, transform.position.z);
		}
	}
} // namespace Engine::Components

#include "assets/AssetManager.inl"