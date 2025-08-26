//
// Created by gabe on 8/18/25.
//

#include "Particle.h"
#include "core/EngineData.h"
#include "ParticleManager.h"
#include "utils/Utils.h"
#include <codecvt>
#include <locale>
#include <string>

namespace Engine {
	bool Particle::LoadFromFile(const std::string& path)
	{
		std::u16string  utf16 = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.from_bytes(path);
		const char16_t* raw   = utf16.c_str();
		effect_               = Effekseer::Effect::Create(GetParticleManager().GetManager(), raw);

		if (!effect_) {
			ENGINE_WARN("Failed to load particle effect: {}", path);
		}

		path_ = path;
		name  = GetFileName(path);
		return true;
	}
} // namespace Engine