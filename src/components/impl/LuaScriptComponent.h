//
// Created by gabe on 6/29/25.
//

#ifndef CPP_ENGINE_LUASCRIPTCOMPONENT_H
#define CPP_ENGINE_LUASCRIPTCOMPONENT_H

#include <utility>

#include "components/Components.h"

#include <cereal/cereal.hpp>
#include <cereal/types/variant.hpp>
#include <sol/environment.hpp>


#include "core/EntityHandle.h"

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
	                                    TextureHandle,
	                                    ModelHandle,
	                                    MaterialHandle,
	                                    SceneHandle,
	                                    AssetHandle<Terrain ::TerrainTile>,
	                                    ParticleHandle,
	                                    SoundHandle,
	                                    EntityHandle,
	                                    std::vector<EntityHandle>,
	                                    std::vector<TextureHandle>,
	                                    std::vector<ModelHandle>,
	                                    std::vector<MaterialHandle>,
	                                    std::vector<SceneHandle>,
	                                    std::vector<TerrainHandle>,
	                                    std::vector<ParticleHandle>,
	                                    SoundHandleList>;

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
			sol::function                                   lateUpdate; // after physics (camera follow, etc.)
			sol::function                                   collisionEnter;
			sol::function                                   playerCollisionEnter;
			std::unordered_map<std::string, ScriptVariable> cppVariables;
			
			// Track event subscriptions for auto-cleanup
			std::vector<uint32_t> subscriptionIDs;
			void UnsubscribeAll();
		};
	} // namespace Components
} // namespace Engine
#endif // CPP_ENGINE_LUASCRIPTCOMPONENT_H
