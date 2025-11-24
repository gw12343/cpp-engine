//
// Created by gabe on 11/24/25.
//

#ifndef CPP_ENGINE_EVENTBUS_H
#define CPP_ENGINE_EVENTBUS_H

#include <string>
#include <unordered_map>
#include <vector>
#include <variant>
#include <mutex>
#include <sol/sol.hpp>

#include "core/Entity.h"
#include "assets/AssetHandle.h"
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

	// Event data type - can hold various types of data to pass with events
	using EventData = std::variant<std::monostate, // No data
	                                float,
	                                std::string,
	                                glm::vec3,
	                                int,
	                                bool,
	                                AssetHandle<Texture>,
	                                AssetHandle<Rendering::Model>,
	                                AssetHandle<Material>,
	                                AssetHandle<Scene>,
	                                AssetHandle<Terrain::TerrainTile>,
	                                AssetHandle<Particle>,
	                                AssetHandle<Audio::SoundBuffer>,
	                                EntityHandle,
	                                Entity>;

	class EventBus {
	  public:
		EventBus();
		~EventBus() = default;

		// Subscribe to an event with a Lua callback
		void Subscribe(const std::string& eventName, sol::function callback);

		// Unsubscribe from an event (removes all matching callbacks)
		void Unsubscribe(const std::string& eventName, sol::function callback);

		// Publish an event with optional data (queues for later dispatch)
		void Publish(const std::string& eventName);
		void Publish(const std::string& eventName, EventData data);

		// Publish an event with Entity parameter (helper overload)
		void PublishEntityEvent(const std::string& eventName, Entity& entity);

		// Dispatch all queued events (call this during update loop)
		void DispatchEvents();

		// Clear all subscriptions (useful for cleanup)
		void ClearAllSubscriptions();

	  private:
		struct QueuedEvent {
			std::string eventName;
			EventData   data;
		};

		// Map of event names to list of callbacks
		std::unordered_map<std::string, std::vector<sol::function>> m_subscribers;

		// Queue of events to be dispatched
		std::vector<QueuedEvent> m_eventQueue;

		// Mutex for thread safety
		std::mutex m_mutex;
	};

} // namespace Engine

#endif // CPP_ENGINE_EVENTBUS_H
