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

		// Compile Lua scripts to bytecode
	sol::state lua;
	
	for (const auto& entry : fs::recursive_directory_iterator(scriptsDir)) {
		if (fs::is_regular_file(entry.status()) && entry.path().extension() == ".lua") {
			const fs::path& srcPath      = entry.path();
			fs::path        relativePath = fs::relative(srcPath, scriptsDir);
			fs::path        outPath      = outScriptsDir / relativePath;
			
			// Change extension from .lua to .luac
			outPath.replace_extension(".luac");
			
			// Ensure parent directory exists
			fs::create_directories(outPath.parent_path());
			
			try {
				// Load the Lua script
				auto result = lua.load_file(srcPath.string());
				
				if (!result.valid()) {
					sol::error err = result;
					GetDefaultLogger()->error("Failed to compile script {}: {}", srcPath.c_str(), err.what());
					continue;
				}
				
				// Get the compiled function
				sol::protected_function func = result;
				
				// Dump bytecode to file
				std::ofstream outFile(outPath, std::ios::binary);
				if (!outFile) {
					GetDefaultLogger()->error("Failed to open output file {}", outPath.c_str());
					continue;
				}
				
				// Use Lua C API to dump the bytecode
				lua_State* L = lua.lua_state();
				
				// Push the function onto the stack
				func.push();
				
				// Lambda for lua_dump writer
				auto writer = [](lua_State* L, const void* p, size_t sz, void* ud) -> int {
					std::ofstream* file = static_cast<std::ofstream*>(ud);
					file->write(static_cast<const char*>(p), sz);
					return file->good() ? 0 : 1;
				};
				
				int dumpResult = lua_dump(L, writer, &outFile, 0);
				lua_pop(L, 1); // Pop the function
				
				if (dumpResult != 0) {
					GetDefaultLogger()->error("Failed to dump bytecode for {}", srcPath.c_str());
					continue;
				}
				
				outFile.close();
				GetDefaultLogger()->info("Compiled Script {} -> {}", srcPath.c_str(), outPath.c_str());
			}
			catch (const std::exception& e) {
				GetDefaultLogger()->error("Exception compiling {}: {}", srcPath.c_str(), e.what());
			}
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