#include "Text3DComponent.h"

#include "core/Entity.h"
#include "scripting/ScriptManager.h"
#include "rendering/ui/InspectorUI.h"

#include "misc/cpp/imgui_stdlib.h"
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>

namespace Engine::Components {

	void Text3DComponent::OnAdded(Entity& /*entity*/) {}
	void Text3DComponent::OnRemoved(Entity& /*entity*/) {}

	void Text3DComponent::RenderInspector(Entity& /*entity*/)
	{
		LeftLabelInputText("Text", &text);
		LeftLabelInputText("Font", &fontPath);
		ImGui::SameLine();
		BrowsePathButton("font", "ttf,otf", "resources/fonts", &fontPath);
		LeftLabelDragFloat("Size", &size, 0.01f);
		if (size < 0.001f) size = 0.001f;

		ImGui::TextUnformatted("Atlas Px");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(-1);
		if (ImGui::DragInt("##AtlasPx", &atlasPixelHeight, 1, 16, 256)) {
			atlasPixelHeight = std::clamp(atlasPixelHeight, 16, 256);
		}

		LeftLabelColorEdit3("Color RGB", glm::value_ptr(color));
		LeftLabelSliderFloat("Alpha", &color.a, 0.f, 1.f);
		LeftLabelCheckbox("Billboard", &billboard);
		if (billboard) {
			LeftLabelCheckbox("Y Lock", &billboardYLock);
		}

		const char* alignItems[] = {"Left", "Center", "Right"};
		LeftLabelCombo("Align", &alignment, alignItems, 3);

		LeftLabelDragFloat("Letter Spacing", &letterSpacing, 0.001f);
		LeftLabelSliderFloat("Outline", &outlineWidth, 0.f, 0.5f);
		LeftLabelColorEdit3("Outline Color", glm::value_ptr(outlineColor));
	}

	void Text3DComponent::AddBindings()
	{
		auto& lua = GetScriptManager().lua;

		lua.new_usertype<Text3DComponent>(
		    "Text3DComponent",
		    "text", &Text3DComponent::text,
		    "fontPath", &Text3DComponent::fontPath,
		    "size", &Text3DComponent::size,
		    "atlasPixelHeight", &Text3DComponent::atlasPixelHeight,
		    "color", &Text3DComponent::color,
		    "billboard", &Text3DComponent::billboard,
		    "billboardYLock", &Text3DComponent::billboardYLock,
		    "alignment", &Text3DComponent::alignment,
		    "letterSpacing", &Text3DComponent::letterSpacing,
		    "outlineWidth", &Text3DComponent::outlineWidth,
		    "outlineColor", &Text3DComponent::outlineColor);
	}

} // namespace Engine::Components
