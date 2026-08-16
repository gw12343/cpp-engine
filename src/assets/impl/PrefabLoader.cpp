#include "PrefabLoader.h"

#include "assets/SerializedEntity.h"
#include "core/EngineData.h"
#include "utils/Logger.h"

#include <nlohmann/json.hpp>

#include <cereal/archives/json.hpp>
#include <cereal/types/common.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/optional.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/variant.hpp>
#include <cereal/types/vector.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>

namespace Engine {
	std::unique_ptr<Prefab> PrefabLoader::LoadFromFile(const std::string& path)
	{
		auto prefab = std::make_unique<Prefab>();
		prefab->m_name = std::filesystem::path(path).stem().string();

		std::ifstream is(path);
		if (!is) {
			GetDefaultLogger()->error("[Prefab] Failed to open {}", path);
			return prefab;
		}

		nlohmann::json j;
		try {
			is >> j;
		}
		catch (const std::exception& e) {
			GetDefaultLogger()->error("[Prefab] JSON parse failed for {}: {}", path, e.what());
			return prefab;
		}

		EnsureSerializedEntityKeys(j);

		try {
			std::stringstream ss;
			ss << j.dump();
			cereal::JSONInputArchive archive(ss);
			archive(cereal::make_nvp("name", prefab->m_name), cereal::make_nvp("rootGuid", prefab->rootGuid), cereal::make_nvp("entities", prefab->entities));
		}
		catch (const std::exception& e) {
			GetDefaultLogger()->error("[Prefab] cereal load failed for {}: {}", path, e.what());
		}

		if (prefab->m_name.empty()) {
			prefab->m_name = std::filesystem::path(path).stem().string();
		}
		return prefab;
	}

	bool PrefabLoader::SaveToFile(const Prefab& prefab, const std::string& path)
	{
		std::error_code ec;
		std::filesystem::create_directories(std::filesystem::path(path).parent_path(), ec);

		std::ofstream os(path);
		if (!os) {
			GetDefaultLogger()->error("[Prefab] Failed to write {}", path);
			return false;
		}

		try {
			cereal::JSONOutputArchive archive(os);
			std::string               name = prefab.m_name;
			std::string               root = prefab.rootGuid;
			auto                      ents = prefab.entities;
			archive(cereal::make_nvp("name", name), cereal::make_nvp("rootGuid", root), cereal::make_nvp("entities", ents));
		}
		catch (const std::exception& e) {
			GetDefaultLogger()->error("[Prefab] cereal save failed for {}: {}", path, e.what());
			return false;
		}
		return true;
	}
} // namespace Engine
