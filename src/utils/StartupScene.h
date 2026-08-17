#pragma once

#include <cctype>
#include <filesystem>
#include <fstream>
#include <string>

namespace Engine {

	inline constexpr const char* kStartupSceneFile = "scenes/startup";
	inline constexpr const char* kDefaultGameScene = "scenes/scene1.bin";

	inline std::string SceneBinPathFromSource(const std::string& sourcePath)
	{
		std::filesystem::path src(sourcePath);
		std::string           stem = src.stem().string();
		if (stem.empty() || stem.front() == '.') {
			stem = "scene1";
		}
		return (std::filesystem::path("scenes") / (stem + ".bin")).generic_string();
	}

	inline std::string ReadStartupScenePath()
	{
		std::ifstream in(kStartupSceneFile);
		if (!in) {
			return kDefaultGameScene;
		}
		std::string line;
		std::getline(in, line);
		while (!line.empty() && std::isspace(static_cast<unsigned char>(line.back()))) {
			line.pop_back();
		}
		size_t start = 0;
		while (start < line.size() && std::isspace(static_cast<unsigned char>(line[start]))) {
			++start;
		}
		if (start > 0) {
			line.erase(0, start);
		}
		return line.empty() ? kDefaultGameScene : line;
	}

	inline void WriteStartupScenePath(const std::string& sceneBinPath)
	{
		std::error_code ec;
		std::filesystem::create_directories("scenes", ec);
		std::ofstream out(kStartupSceneFile, std::ios::trunc);
		if (out) {
			out << sceneBinPath << '\n';
		}
	}

} // namespace Engine
