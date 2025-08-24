//
// Created by gabe on 8/24/25.
//

#include "ConsoleWindow.h"
#include "imgui.h"

namespace Engine {
	static bool                      autoScroll  = true;
	static spdlog::level::level_enum filterLevel = spdlog::level::trace;


	void DrawConsoleWindow(std::shared_ptr<ImGuiLogSink> sink, bool* pOpen)
	{
		if (!ImGui::Begin("Console", pOpen)) {
			ImGui::End();
			return;
		}

		// Toolbar
		if (ImGui::Button("Clear")) {
			sink->clear();
		}
		ImGui::SameLine();
		ImGui::Checkbox("Auto-scroll", &autoScroll);

		ImGui::SameLine();
		// Log level filter
		if (ImGui::BeginCombo("Level", spdlog::level::to_string_view(filterLevel).data())) {
			for (int lvl = spdlog::level::trace; lvl <= spdlog::level::critical; ++lvl) {
				bool selected = (filterLevel == lvl);
				if (ImGui::Selectable(spdlog::level::to_string_view((spdlog::level::level_enum) lvl).data(), selected)) filterLevel = (spdlog::level::level_enum) lvl;
				if (selected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		ImGui::Separator();

		// Log output region
		ImGui::BeginChild("LogRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

		const auto& messages = sink->messages();
		for (auto& msg : messages) {
			if (msg.level < filterLevel) continue; // skip messages below filter

			ImVec4 color;
			switch (msg.level) {
				case spdlog::level::trace:
					color = ImVec4(0.5f, 0.5f, 0.5f, 1.f);
					break;
				case spdlog::level::debug:
					color = ImVec4(0.6f, 0.8f, 1.f, 1.f);
					break;
				case spdlog::level::info:
					color = ImVec4(1.f, 1.f, 1.f, 1.f);
					break;
				case spdlog::level::warn:
					color = ImVec4(1.f, 1.f, 0.3f, 1.f);
					break;
				case spdlog::level::err:
					color = ImVec4(1.f, 0.4f, 0.4f, 1.f);
					break;
				case spdlog::level::critical:
					color = ImVec4(1.f, 0.f, 0.f, 1.f);
					break;
				default:
					color = ImVec4(1.f, 1.f, 1.f, 1.f);
					break;
			}

			ImGui::PushStyleColor(ImGuiCol_Text, color);
			ImGui::TextUnformatted(msg.message.c_str());
			ImGui::PopStyleColor();
		}

		if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);

		ImGui::EndChild();

		ImGui::End();
	}
} // namespace Engine