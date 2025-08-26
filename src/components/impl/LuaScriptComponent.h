//
// Created by gabe on 6/29/25.
//

#ifndef CPP_ENGINE_LUASCRIPTCOMPONENT_H
#define CPP_ENGINE_LUASCRIPTCOMPONENT_H

#include <utility>

#include "components/Components.h"

#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/variant.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/unordered_map.hpp>

#include "assets/AssetHandle.h"

namespace Engine {
	class Texture;
	class Material;
	class Scene;
	class Particle;
	namespace Terrain {
		class TerrainTile;
	}
	namespace Rendering {
		class Model;
	}

	namespace Audio {
		class SoundBuffer;
	}
} // namespace Engine

namespace Engine {
	using ScriptVariable = std::variant<float,
	                                    std::string,
	                                    glm::vec3,
	                                    int,
	                                    bool,
	                                    AssetHandle<Texture>,
	                                    AssetHandle<Rendering::Model>,
	                                    AssetHandle<Material>,
	                                    AssetHandle<Scene>,
	                                    AssetHandle<Terrain ::TerrainTile>,
	                                    AssetHandle<Particle>,
	                                    AssetHandle<Audio::SoundBuffer>>;

	namespace Components {

		class LuaScript : public Component {
		  public:
			LuaScript() = default;
			explicit LuaScript(std::string path) : scriptPath(std::move(path)) {}

			template <class Archive>
			void serialize(Archive& ar)
			{
				ar(cereal::make_nvp("scriptPath", scriptPath), cereal::make_nvp("variables", cppVariables));
			}


			void SyncFromLua();
			void SyncToLua();

			void        OnAdded(Entity& entity) override;
			void        OnRemoved(Entity& entity) override;
			void        RenderInspector(Entity& entity) override;
			void        LoadScript(Engine::Entity& entity, std::string path);
			void        OnCollisionEnter(Entity& other);
			void        OnPlayerCollisionEnter();
			static void AddBindings();

			bool             hasStarted = false;
			std::string      scriptPath;
			sol::environment env;

			sol::table                                      variables;
			sol::function                                   start;
			sol::function                                   update;
			sol::function                                   collisionEnter;
			sol::function                                   playerCollisionEnter;
			std::unordered_map<std::string, ScriptVariable> cppVariables;
		};
	} // namespace Components
} // namespace Engine
#endif // CPP_ENGINE_LUASCRIPTCOMPONENT_H
