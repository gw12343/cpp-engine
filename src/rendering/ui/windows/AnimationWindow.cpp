//
// Created by gabe on 8/24/25.
//

#include "AnimationWindow.h"

#include "core/EngineData.h"
#include "core/Window.h"
#include "animation/AnimationManager.h"

#include <cctype>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>
#include <nfd.h>

namespace Engine {
	namespace fs = std::filesystem;

	namespace {
		struct FbxImportLog {
			std::vector<std::string> lines;

			void Clear() { lines.clear(); }

			void Add(const std::string& line)
			{
				lines.push_back(line);
				if (lines.size() > 40) {
					lines.erase(lines.begin(), lines.begin() + static_cast<std::ptrdiff_t>(lines.size() - 40));
				}
				spdlog::info("[fbx2ozz] {}", line);
			}

			void Error(const std::string& line)
			{
				lines.push_back(std::string("ERROR: ") + line);
				if (lines.size() > 40) {
					lines.erase(lines.begin(), lines.begin() + static_cast<std::ptrdiff_t>(lines.size() - 40));
				}
				spdlog::error("[fbx2ozz] {}", line);
			}
		};

		FbxImportLog& GetFbxImportLog()
		{
			static FbxImportLog log;
			return log;
		}

		bool EndsWithIgnoreCase(const std::string& value, const std::string& suffix)
		{
			if (value.size() < suffix.size()) return false;
			for (size_t i = 0; i < suffix.size(); ++i) {
				const char a = static_cast<char>(std::tolower(static_cast<unsigned char>(value[value.size() - suffix.size() + i])));
				const char b = static_cast<char>(std::tolower(static_cast<unsigned char>(suffix[i])));
				if (a != b) return false;
			}
			return true;
		}

		bool IsFbxPath(const fs::path& path)
		{
			return EndsWithIgnoreCase(path.extension().string(), ".fbx");
		}

		std::string MakeUniqueTempName(const std::string& stem)
		{
			const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
			                    std::chrono::steady_clock::now().time_since_epoch())
			                    .count();
			return "cpp_engine_fbx2ozz_" + stem + "_" + std::to_string(ms);
		}

		int RunFbx2OzzInDirectory(const fs::path& workDir, const fs::path& fbxFileName)
		{
			// fbx2ozz writes outputs relative to CWD by default; run inside temp dir.
			const fs::path previous = fs::current_path();
			std::error_code ec;
			fs::current_path(workDir, ec);
			if (ec) {
				GetFbxImportLog().Error("Failed to enter temp dir: " + workDir.string() + " (" + ec.message() + ")");
				return -1;
			}

			// fbx2ozz is expected to be on PATH.
			const std::string command = "fbx2ozz --file=\"" + fbxFileName.filename().string() + "\"";
			GetFbxImportLog().Add("Running: " + command + "  (cwd=" + workDir.string() + ")");
			const int code = std::system(command.c_str());

			fs::current_path(previous, ec);
			if (ec) {
				GetFbxImportLog().Error("Failed to restore working directory: " + ec.message());
			}
			return code;
		}

		bool ImportFbxToAnimationsFolder(const fs::path& fbxPath)
		{
			auto& log = GetFbxImportLog();

			if (!fs::exists(fbxPath)) {
				log.Error("File does not exist: " + fbxPath.string());
				return false;
			}
			if (!IsFbxPath(fbxPath)) {
				log.Error("Not an .fbx file: " + fbxPath.string());
				return false;
			}

			const std::string originalStem = fbxPath.stem().string();
			if (originalStem.empty()) {
				log.Error("Could not derive output name from: " + fbxPath.string());
				return false;
			}

			const fs::path tempRoot = fs::temp_directory_path() / MakeUniqueTempName(originalStem);
			std::error_code ec;
			fs::create_directories(tempRoot, ec);
			if (ec) {
				log.Error("Failed to create temp folder: " + tempRoot.string() + " (" + ec.message() + ")");
				return false;
			}

			const fs::path tempFbx = tempRoot / fbxPath.filename();
			fs::copy_file(fbxPath, tempFbx, fs::copy_options::overwrite_existing, ec);
			if (ec) {
				log.Error("Failed to copy FBX to temp: " + ec.message());
				return false;
			}

			log.Add("Temp extract: " + tempRoot.string());
			log.Add("Source FBX: " + fbxPath.string());

			const int code = RunFbx2OzzInDirectory(tempRoot, tempFbx);
			if (code != 0) {
				log.Error("fbx2ozz failed with exit code " + std::to_string(code) +
				          " (is fbx2ozz.exe on PATH?)");
				// Keep temp dir for inspection on failure
				return false;
			}

			// Collect .ozz outputs that are not skeleton.ozz
			std::vector<fs::path> animOutputs;
			for (const auto& entry : fs::directory_iterator(tempRoot, ec)) {
				if (ec) break;
				if (!entry.is_regular_file()) continue;
				const fs::path& p = entry.path();
				if (!EndsWithIgnoreCase(p.extension().string(), ".ozz")) continue;
				if (EndsWithIgnoreCase(p.filename().string(), "skeleton.ozz")) continue;
				animOutputs.push_back(p);
			}

			if (animOutputs.empty()) {
				log.Error("fbx2ozz finished but no non-skeleton .ozz was produced in " + tempRoot.string());
				return false;
			}

			if (animOutputs.size() > 1) {
				log.Add("Multiple animation outputs found; using the first:");
				for (const auto& p : animOutputs) {
					log.Add("  - " + p.filename().string());
				}
			}

			const fs::path destDir  = fs::path("resources") / "animations";
			const fs::path destFile = destDir / (originalStem + ".anim");
			fs::create_directories(destDir, ec);
			if (ec) {
				log.Error("Failed to create " + destDir.string() + ": " + ec.message());
				return false;
			}

			fs::copy_file(animOutputs.front(), destFile, fs::copy_options::overwrite_existing, ec);
			if (ec) {
				log.Error("Failed to copy animation to " + destFile.string() + ": " + ec.message());
				return false;
			}

			log.Add("Copied " + animOutputs.front().filename().string() + " → " + destFile.generic_string());

			// Clean temp on success
			fs::remove_all(tempRoot, ec);
			if (ec) {
				log.Add("Note: could not fully remove temp folder: " + tempRoot.string());
			} else {
				log.Add("Cleaned temp folder.");
			}

			return true;
		}

		void ProcessDroppedFbxFiles(const std::vector<std::string>& paths)
		{
			for (const auto& pathStr : paths) {
				const fs::path path(pathStr);
				if (!IsFbxPath(path)) {
					GetFbxImportLog().Add("Skipped (not .fbx): " + pathStr);
					continue;
				}
				ImportFbxToAnimationsFolder(path);
			}
		}

		void DrawFbxImportUtility()
		{
			if (!ImGui::CollapsingHeader("FBX → OZZ Import (fbx2ozz)", ImGuiTreeNodeFlags_DefaultOpen)) {
				return;
			}

			ImGui::TextWrapped(
			    "Drop a .fbx file onto the zone below, or use Browse. "
			    "Runs fbx2ozz.exe (must be on PATH) and copies the clip to resources/animations/{name}.anim");

			if (ImGui::Button("Browse .fbx...")) {
				nfdchar_t*  outPath = nullptr;
				if (NFD_OpenDialog("fbx", nullptr, &outPath) == NFD_OKAY && outPath) {
					ImportFbxToAnimationsFolder(fs::path(outPath));
					free(outPath);
				}
			}

			ImGui::Spacing();

			const ImVec2 dropSize(0.0f, 90.0f);
			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
			ImGui::BeginChild("##fbx_drop_zone", dropSize, true);

			const bool zoneHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
			if (zoneHovered) {
				ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.5f, 1.f), "Drop .fbx here");
			} else {
				ImGui::TextDisabled("Drop .fbx here");
			}
			ImGui::TextWrapped("Output: resources/animations/<name>.ozz  (skeleton.ozz is ignored)");

			// Accept OS drops while the mouse is over this zone (drop callback ran during PollEvents).
			if (zoneHovered && GetWindow().HasDroppedFiles()) {
				const auto dropped = GetWindow().ConsumeDroppedFiles();
				ProcessDroppedFbxFiles(dropped);
			}

			ImGui::EndChild();
			ImGui::PopStyleColor();

			// If the Animation window is focused, also accept FBX drops anywhere on it
			// (helps when the drop zone is small / hard to hit).
			if (!zoneHovered && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
			    GetWindow().HasDroppedFiles()) {
				const auto dropped = GetWindow().ConsumeDroppedFiles();
				ProcessDroppedFbxFiles(dropped);
			}

			if (ImGui::Button("Clear Log")) {
				GetFbxImportLog().Clear();
			}

			ImGui::Separator();
			ImGui::BeginChild("##fbx_import_log", ImVec2(0, 140), true, ImGuiWindowFlags_HorizontalScrollbar);
			for (const auto& line : GetFbxImportLog().lines) {
				const bool isErr = line.rfind("ERROR:", 0) == 0;
				if (isErr) {
					ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f), "%s", line.c_str());
				} else {
					ImGui::TextUnformatted(line.c_str());
				}
			}
			if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
				ImGui::SetScrollHereY(1.0f);
			}
			ImGui::EndChild();
		}
	} // namespace

	void DrawAnimationWindow()
	{
		ImGui::Begin("Animation");

		if (ImGui::CollapsingHeader("Rendering Options")) {
			auto& animManager    = GetAnimationManager();
			auto& draw_skeleton  = animManager.GetDrawSkeleton();
			auto& draw_mesh      = animManager.GetDrawMesh();
			auto& render_options = animManager.GetRenderOptions();

			ImGui::Checkbox("Show Skeleton", &draw_skeleton);
			ImGui::Checkbox("Show Mesh", &draw_mesh);
			ImGui::Separator();

			ImGui::Checkbox("Show triangles", &render_options.triangles);
			ImGui::Checkbox("Show texture", &render_options.texture);
			ImGui::Checkbox("Show vertices", &render_options.vertices);
			ImGui::Checkbox("Show normals", &render_options.normals);
			ImGui::Checkbox("Show tangents", &render_options.tangents);
			ImGui::Checkbox("Show binormals", &render_options.binormals);
			ImGui::Checkbox("Show colors", &render_options.colors);
			ImGui::Checkbox("Wireframe", &render_options.wireframe);
			ImGui::Checkbox("Skip skinning", &render_options.skip_skinning);
		}

		ImGui::Separator();
		DrawFbxImportUtility();

		ImGui::End();
	}
} // namespace Engine
