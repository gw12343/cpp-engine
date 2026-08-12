#pragma once

#include "components/Components.h"

#include <cereal/cereal.hpp>
#include <cereal/types/string.hpp>
#include <string>
#include <glm/glm.hpp>

namespace Engine::Components {

	// World-space SDF text label (museum / gym signage).
	// Rendered forward-transparent after deferred lighting.
	class Text3DComponent : public Component {
	  public:
		std::string text     = "Label";
		std::string fontPath = "resources/fonts/Roboto-Regular.ttf";

		// Approximate height of a capital letter in world units.
		float size = 0.25f;

		// Atlas bake size (pixels). Higher = sharper when large / close.
		int atlasPixelHeight = 64;

		glm::vec4 color{1.f, 1.f, 1.f, 1.f};

		// Face the camera each frame.
		bool billboard = true;
		// When billboarding, keep text upright (lock world +Y).
		bool billboardYLock = true;

		// 0 = left, 1 = center, 2 = right (relative to entity origin).
		int alignment = 1;

		// Extra spacing between glyphs in world units.
		float letterSpacing = 0.f;

		// SDF outline (0 = none). Typical range 0.0 – 0.4.
		float     outlineWidth = 0.08f;
		glm::vec3 outlineColor{0.f, 0.f, 0.f};

		Text3DComponent() = default;

		template <class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("text", text),
			   cereal::make_nvp("fontPath", fontPath),
			   cereal::make_nvp("size", size),
			   cereal::make_nvp("atlasPixelHeight", atlasPixelHeight),
			   cereal::make_nvp("color", color),
			   cereal::make_nvp("billboard", billboard),
			   cereal::make_nvp("billboardYLock", billboardYLock),
			   cereal::make_nvp("alignment", alignment),
			   cereal::make_nvp("letterSpacing", letterSpacing),
			   cereal::make_nvp("outlineWidth", outlineWidth),
			   cereal::make_nvp("outlineColor", outlineColor));
		}

		void OnAdded(Entity& entity) override;
		void OnRemoved(Entity& entity) override;
		void RenderInspector(Entity& entity) override;

		static void AddBindings();
	};

} // namespace Engine::Components
