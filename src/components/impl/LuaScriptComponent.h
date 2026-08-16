//
// Created by gabe on 6/29/25.
//

#ifndef CPP_ENGINE_LUASCRIPTCOMPONENT_H
#define CPP_ENGINE_LUASCRIPTCOMPONENT_H

#include <utility>

#include "components/Components.h"

#include <cereal/cereal.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/variant.hpp>
#include <unordered_map>
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
	// Indices are serialized into scenes — append new types only at the end.
	using ScriptVariable = std::variant<float,                                    // 0
	                                    std::string,                              // 1
	                                    glm::vec3,                                // 2
	                                    int,                                      // 3
	                                    bool,                                     // 4
	                                    TextureHandle,                            // 5
	                                    ModelHandle,                              // 6
	                                    MaterialHandle,                           // 7
	                                    SceneHandle,                              // 8
	                                    AssetHandle<Terrain::TerrainTile>,        // 9
	                                    ParticleHandle,                           // 10
	                                    SoundHandle,                              // 11
	                                    EntityHandle,                             // 12
	                                    std::vector<EntityHandle>,                // 13
	                                    std::vector<TextureHandle>,               // 14
	                                    std::vector<ModelHandle>,                 // 15
	                                    std::vector<MaterialHandle>,              // 16
	                                    std::vector<SceneHandle>,                 // 17
	                                    std::vector<TerrainHandle>,               // 18
	                                    std::vector<ParticleHandle>,              // 19
	                                    SoundHandleList,                          // 20
	                                    AnimationHandle,                          // 21
	                                    AnimationHandleList,                      // 22
	                                    SkeletonReference,                        // 23
	                                    SkeletonHandleList,                       // 24
	                                    PrefabHandle,                             // 25
	                                    PrefabHandleList>;                        // 26

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
