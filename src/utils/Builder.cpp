//
// Created by gabe on 8/27/25.
//


#include "Builder.h"



#include <cstdlib>
#include <vector>
#include "core/EngineData.h"
#include "assets/impl/BinarySceneLoader.h"
#include <sol/sol.hpp>
#include <fstream>

#include "core/SceneManager.h"
#include "core/ProjectSettings.h"
#include "core/EnginePaths.h"
#include "rendering/ui/EditorSession.h"

namespace Engine {
	namespace fs = std::filesystem;


	void CopyResourcesAssets(const fs::path& engineRoot, const fs::path& projectRoot, const fs::path& outRoot)
	{
        fs::path sourceResources = engineRoot / "resources";
		fs::path outResources    = outRoot / "resources";
        fs::path sourceAssets = projectRoot / "assets";
        fs::path outAssets    = outRoot / "assets";

		if (!fs::exists(sourceResources) || !fs::is_directory(sourceResources)) {
			std::cerr << "Source resources folder does not exist!\n";
		}else{
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
        if (!fs::exists(sourceAssets) || !fs::is_directory(sourceAssets)) {
			std::cerr << "Source assets folder does not exist!\n";
		}else {
            try {
                // Create out/resources folder if it doesn't exist
                fs::create_directories(outAssets);

                for (auto& entry : fs::recursive_directory_iterator(sourceAssets)) {
                    fs::path relativePath = fs::relative(entry.path(), sourceAssets);

                    // Skip anything under "engine" folder
                    if (!relativePath.empty() && relativePath.begin()->string() == "engine") continue;

                    fs::path destPath = outAssets / relativePath;

                    if (fs::is_directory(entry.status())) {
                        fs::create_directories(destPath);
                    }
                    else if (fs::is_regular_file(entry.status())) {
                        fs::copy_file(entry.path(), destPath, fs::copy_options::overwrite_existing);
                    }
                }

                std::cout << "Resources copied successfully to: " << outAssets << "\n";
            }
            catch (const fs::filesystem_error& e) {
                std::cerr << "Error copying resources: " << e.what() << "\n";
            }
        }


	}


	void CreateOutputDirectory(const fs::path& outPath)
	{
		try {
			if (!std::filesystem::exists(outPath)) {
				std::filesystem::create_directory(outPath);
				GetDefaultLogger()->info("Created folder: {}", outPath.string());
			}
			else {
				GetDefaultLogger()->info("Folder already exists: {}", outPath.string());
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

		const fs::path engineRoot  = GetEnginePaths().EngineRoot();
		const fs::path projectRoot = GetProject().IsOpen() ? fs::path(GetProject().Root()) : fs::current_path();

		// Engine resources stay global; game content comes from the project folder.
		CopyResourcesAssets(engineRoot, projectRoot, outPath);

		// Pre-compile scripts

		fs::path scriptsDir    = projectRoot / "scripts";
		fs::path outScriptsDir = outPath / "scripts";

		fs::create_directories(outScriptsDir);

		if (!fs::exists(scriptsDir) || !fs::is_directory(scriptsDir)) {
			GetDefaultLogger()->warn("Project scripts/ folder does not exist");
		}

		// Compile Lua scripts to bytecode
	sol::state lua;

	if (fs::exists(scriptsDir) && fs::is_directory(scriptsDir))
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
					GetDefaultLogger()->error("Failed to compile script {}: {}", srcPath.string(), err.what());
					continue;
				}
				
				// Get the compiled function
				sol::protected_function func = result;
				
				// Dump bytecode to file
				std::ofstream outFile(outPath, std::ios::binary);
				if (!outFile) {
					GetDefaultLogger()->error("Failed to open output file {}", outPath.string());
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
					GetDefaultLogger()->error("Failed to dump bytecode for {}", srcPath.string());
					continue;
				}
				
				outFile.close();
				GetDefaultLogger()->info("Compiled Script {} -> {}", srcPath.string(), outPath.string());
			}
			catch (const std::exception& e) {
				GetDefaultLogger()->error("Exception compiling {}: {}", srcPath.string(), e.what());
			}
		}
	}

		// Pack every project build scene. Scene 0 is the game startup scene.
		fs::path outScenesDir = outPath / "scenes";
		fs::create_directories(outScenesDir);

		auto packScene = [&](const std::string& sourcePath) {
			const fs::path diskPath = GetEnginePaths().Resolve(sourcePath);
			if (sourcePath.empty() || !fs::exists(diskPath)) {
				GetDefaultLogger()->warn("Skipping missing project scene {}", sourcePath);
				return;
			}
			SceneHandle handle = GetAssetManager().Load<Scene>(sourcePath);
			if (!handle.IsValid() || !GetAssetManager().Get(handle)) {
				GetDefaultLogger()->error("Failed to load scene for pack: {}", sourcePath);
				return;
			}
			fs::path binRel = fs::path(sourcePath);
			if (binRel.extension() == ".json") {
				binRel.replace_extension(".bin");
			}
			const fs::path outBin  = outPath / binRel;
			const fs::path projBin = fs::current_path() / binRel;
			fs::create_directories(outBin.parent_path());
			BinarySceneLoader::SerializeScene(handle, outBin.string());
			GetDefaultLogger()->info("Packed scene {} -> {}", sourcePath, outBin.string());

			std::error_code ec;
			fs::create_directories(projBin.parent_path(), ec);
			fs::copy_file(outBin, projBin, fs::copy_options::overwrite_existing, ec);
			if (ec) {
				GetDefaultLogger()->warn("Could not copy packed scene to {}: {}", projBin.string(), ec.message());
			}

			if (!(handle == GetSceneManager().GetActiveScene())) {
				GetAssetManager().Unload<Scene>(handle);
			}
		};

		auto& project = GetProject();
		if (project.scenes.empty()) {
			packScene(UI::GetEditor().scenePath);
		}
		else {
			for (const auto& scenePath : project.scenes) {
				packScene(scenePath);
			}
		}

		project.Save();
		try {
			fs::copy_file(GetProject().FilePath(), outPath / "project.json", fs::copy_options::overwrite_existing);
		}
		catch (const std::exception& e) {
			GetDefaultLogger()->error("Failed to copy project.json: {}", e.what());
		}

		auto copyIfExists = [&](const fs::path& from, const fs::path& to) -> bool {
			std::error_code ec;
			if (!fs::exists(from, ec) || !fs::is_regular_file(from, ec)) {
				return false;
			}
			fs::copy_file(from, to, fs::copy_options::overwrite_existing, ec);
			if (ec) {
				GetDefaultLogger()->error("Failed to copy {} -> {}: {}", from.string(), to.string(), ec.message());
				return false;
			}
			GetDefaultLogger()->info("Copied {} -> {}", from.string(), to.string());
			return true;
		};

#ifdef _WIN32
		{
			const char* runtimeDlls[] = {"soft_oal.dll", "assimp.dll", "OpenAL32.dll"};
			const fs::path exeDir(GetEnginePaths().ExecutableDirectory());
			for (const char* dll : runtimeDlls) {
				if (copyIfExists(engineRoot / dll, outPath / dll)) {
					continue;
				}
				if (!exeDir.empty() && copyIfExists(exeDir / dll, outPath / dll)) {
					continue;
				}
				GetDefaultLogger()->warn("Could not find {} next to the engine or editor", dll);
			}
		}
#endif

		fs::path exeName = "cpp-engine_game";
#ifdef _WIN32
		exeName += ".exe";
#endif
#ifdef _DEBUG
		const char* configDir     = "cmake-build-debug";
		const char* vsConfigDir   = "Debug";
#else
		const char* configDir     = "cmake-build-release";
		const char* vsConfigDir   = "Release";
#endif

		std::vector<fs::path> exeCandidates;
		const fs::path        exeDir(GetEnginePaths().ExecutableDirectory());
		if (!exeDir.empty()) {
			exeCandidates.push_back(exeDir / exeName);
		}
		exeCandidates.push_back(engineRoot / configDir / exeName);
		exeCandidates.push_back(engineRoot / "build" / vsConfigDir / exeName);
		exeCandidates.push_back(engineRoot / "build" / exeName);
		exeCandidates.push_back(engineRoot / exeName);

		fs::path sourceExe;
		for (const auto& candidate : exeCandidates) {
			if (fs::exists(candidate) && fs::is_regular_file(candidate)) {
				sourceExe = candidate;
				break;
			}
		}

		fs::path destExe = outPath / exeName;
		if (!sourceExe.empty()) {
			try {
				copyIfExists(sourceExe, destExe);

#ifndef _WIN32
				fs::permissions(destExe, fs::perms::owner_all | fs::perms::group_read | fs::perms::group_exec | fs::perms::others_read | fs::perms::others_exec, fs::perm_options::add);
#endif

				GetDefaultLogger()->info("Running game...");
				std::string command;
#ifdef _WIN32
				command = "cd /d \"" + outPath.string() + "\" && \"" + exeName.string() + "\"";
#else
				command = "cd \"" + outPath.string() + "\" && ./" + exeName.string();
#endif
				std::system(command.c_str());
			} catch (const std::exception& e) {
				GetDefaultLogger()->error("Failed to copy or run executable: {}", e.what());
			}
		} else {
			GetDefaultLogger()->warn("Could not find {} in the editor folder or {} / {}", exeName.string(), configDir, vsConfigDir);
		}

    }
} // namespace Engine

#include "assets/AssetManager.inl"