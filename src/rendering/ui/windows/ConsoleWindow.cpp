//
// Created by gabe on 8/24/25.
//

#include "ConsoleWindow.h"


namespace Engine {
	static bool                      autoScroll  = true;
	static spdlog::level::level_enum filterLevel = spdlog::level::trace;


	void DrawConsoleWindow(std::shared_ptr<ImGuiLogSink> sink, bool* pOpen)
	{
		if (!ImGui::Begin("Console", pOpen)) {
			ImGui::End();
			return;
		}

		static char search[128] = "";

		// Toolbar
		if (ImGui::Button("Clear")) {
			sink->clear();
		}
		ImGui::SameLine();
		ImGui::Checkbox("Auto-scroll", &autoScroll);

		ImGui::SameLine();
		ImGui::SetNextItemWidth(160);
		ImGui::InputTextWithHint("##clog", "Search...", search, IM_ARRAYSIZE(search));

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

		// Log output region — clip to visible rows (full log history is very expensive in ImGui).
		ImGui::BeginChild("LogRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

		const auto& messages = sink->messages();
		// Pre-filter indices so ListClipper only walks visible rows.
		static std::vector<int> visible;
		visible.clear();
		visible.reserve(messages.size());
		for (int i = 0; i < static_cast<int>(messages.size()); ++i) {
			const auto& msg = messages[static_cast<size_t>(i)];
			if (msg.level < filterLevel) continue;
			if (search[0] != '\0' && msg.message.find(search) == std::string::npos) continue;
			visible.push_back(i);
		}

		ImGuiListClipper clipper;
		clipper.Begin(static_cast<int>(visible.size()));
		while (clipper.Step()) {
			for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
				const auto& msg = messages[static_cast<size_t>(visible[static_cast<size_t>(row)])];

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
				if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
					ImGui::SetClipboardText(msg.message.c_str());
				}
			}
		}

		if (ImGui::GetScrollY() < ImGui::GetScrollMaxY() - 8.0f) {
			// user scrolled up
		}
		else if (autoScroll) {
			ImGui::SetScrollHereY(1.0f);
		}
		if (ImGui::IsWindowHovered() && ImGui::GetIO().MouseWheel > 0.0f) {
			autoScroll = false;
		}
		if (ImGui::IsWindowHovered() && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
			autoScroll = true;
		}

		ImGui::EndChild();

		ImGui::End();
	}
} // namespace Engine