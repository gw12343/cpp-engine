//
// Created by gabe on 8/27/25.
//


#include "Builder.h"
#include <string>
#include <iostream>
#include <filesystem>
#include "core/EngineData.h"
#include "assets/impl/BinarySceneLoader.h"
#include <sol/sol.hpp>
#include <fstream>
#include <vector>
#include "core/SceneManager.h"

namespace Engine {
	namespace fs = std::filesystem;


	void CopyResources(const fs::path& sourceRoot, const fs::path& outRoot)
	{
		fs::path sourceResources = sourceRoot / "resources";
		fs::path outResources    = outRoot / "resources";

		if (!fs::exists(sourceResources) || !fs::is_directory(sourceResources)) {
			std::cerr << "Source resources folder does not exist!\n";
			return;
		}

		try {
			// Create out/resources folder if it doesn't exist
			fs::create_directories(outResources);

			for (auto& entry : fs::recursive_directory_iterator(sourceResources)) {
				fs::path relativePath = fs::relative(entry.path(), sourceResources);

				// Skip anything under "engine" folder
				if (!relativePath.empty() && relativePath.begin()->string() == "engine") continue;

				fs::path destPath = outResources / relativePath;

				if (fs::is_directory(entry.status())) {
					fs::create_directories(destPath);
				}
				else if (fs::is_regular_file(entry.status())) {
					fs::copy_file(entry.path(), destPath, fs::copy_options::overwrite_existing);
				}
			}

			std::cout << "Resources copied successfully to: " << outResources << "\n";
		}
		catch (const fs::filesystem_error& e) {
			std::cerr << "Error copying resources: " << e.what() << "\n";
		}
	}


	void CreateOutputDirectory(const fs::path& outPath)
	{
		try {
			if (!std::filesystem::exists(outPath)) {
				std::filesystem::create_directory(outPath);
				GetDefaultLogger()->info("Created folder: {}", outPath.c_str());
			}
			else {
				GetDefaultLogger()->info("Folder already exists: {}", outPath.c_str());
			}
		}
		catch (const std::filesystem::filesystem_error& e) {
			GetDefaultLogger()->error("Error creating folder: {}", e.what());
			return;
		}
	}


	void BuildGame(std::string& path)
	{
		std::filesystem::path p(path);
		if (!(std::filesystem::exists(p) && std::filesystem::is_directory(p))) {
			GetDefaultLogger()->error("Invalid folder {}", path);
		}
		GetDefaultLogger()->info("Building game in {}", path);
		std::filesystem::path outPath = p / "out";

		// Create output directory
		CreateOutputDirectory(outPath);

		// Copy resources
		CopyResources(fs::current_path(), outPath);

		// Pre-compile scripts

		fs::path scriptsDir    = fs::current_path() / "scripts";
		fs::path outScriptsDir = outPath / "scripts";

		fs::create_directories(outScriptsDir);

		if (!fs::exists(scriptsDir) || !fs::is_directory(scriptsDir)) {
			GetDefaultLogger()->warn("scripts/ folder does not exist!");
			return;
		}

		for (const auto& entry : fs::recursive_directory_iterator(scriptsDir)) {
			if (fs::is_regular_file(entry.status()) && entry.path().extension() == ".lua") {
				const fs::path& srcPath      = entry.path();
				fs::path        relativePath = fs::relative(srcPath, scriptsDir);
				fs::path        outPath      = outScriptsDir / relativePath;

				// TODO luac ??
				GetDefaultLogger()->info("Copied Script {} -> {}", srcPath.c_str(), outPath.c_str());
				std::filesystem::copy_file(srcPath, outPath, std::filesystem::copy_options::overwrite_existing);
			}
		}

		// Save scene
		// TODO multi-scene?
		fs::path outScenesDir = outPath / "scenes";

		fs::create_directories(outScenesDir);

		if (!fs::exists(outScenesDir) || !fs::is_directory(outScenesDir)) {
			GetDefaultLogger()->warn("scenes/ folder does not exist!");
			return;
		}

		BinarySceneLoader::SerializeScene(GetSceneManager().GetActiveScene(), (outScenesDir / "scene1.bin").c_str());
	}
} // namespace Engine