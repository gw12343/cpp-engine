#include "Text3DComponent.h"

#include "core/Entity.h"
#include "scripting/ScriptManager.h"
#include "rendering/ui/InspectorUI.h"
#include "rendering/ui/EditorSession.h"

#include "misc/cpp/imgui_stdlib.h"
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>

namespace Engine::Components {

	void Text3DComponent::OnAdded(Entity& /*entity*/) {}
	void Text3DComponent::OnRemoved(Entity& /*entity*/) {}

	void Text3DComponent::RenderInspector(Entity& /*entity*/)
	{
		auto dirty = [](bool changed) {
			if (changed) {
				UI::GetEditor().MarkDirty();
			}
		};

		dirty(LeftLabelInputText("Text", &text));
		dirty(LeftLabelInputText("Font", &fontPath));
		ImGui::SameLine();
		if (BrowsePathButton("font", "ttf,otf", "resources/fonts", &fontPath)) {
			UI::GetEditor().MarkDirty();
		}
		dirty(LeftLabelDragFloat("Size", &size, 0.01f));
		if (size < 0.001f) size = 0.001f;

		ImGui::TextUnformatted("Atlas Px");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(-1);
		if (ImGui::DragInt("##AtlasPx", &atlasPixelHeight, 1, 16, 256)) {
			atlasPixelHeight = std::clamp(atlasPixelHeight, 16, 256);
			UI::GetEditor().MarkDirty();
		}

		dirty(LeftLabelColorEdit3("Color RGB", glm::value_ptr(color)));
		dirty(LeftLabelSliderFloat("Alpha", &color.a, 0.f, 1.f));
		dirty(LeftLabelCheckbox("Billboard", &billboard));
		if (billboard) {
			dirty(LeftLabelCheckbox("Y Lock", &billboardYLock));
		}

		const char* alignItems[] = {"Left", "Center", "Right"};
		alignment = std::clamp(alignment, 0, 2);
		dirty(LeftLabelCombo("Align", &alignment, alignItems, 3));

		dirty(LeftLabelDragFloat("Letter Spacing", &letterSpacing, 0.001f));
		dirty(LeftLabelSliderFloat("Outline", &outlineWidth, 0.f, 0.5f));
		dirty(LeftLabelColorEdit3("Outline Color", glm::value_ptr(outlineColor)));
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
