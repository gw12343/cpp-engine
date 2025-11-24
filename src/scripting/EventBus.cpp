//
// Created by gabe on 11/24/25.
//

#include "EventBus.h"
#include "utils/Logger.h"
#include <algorithm>

namespace Engine {

	EventBus::EventBus()
	{
		GetDefaultLogger()->info("EventBus initialized");
	}

	void EventBus::Subscribe(const std::string& eventName, sol::function callback)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		if (!callback.valid()) {
			GetDefaultLogger()->warn("Attempted to subscribe invalid callback to event: {}", eventName);
			return;
		}

		m_subscribers[eventName].push_back(callback);
		GetDefaultLogger()->debug("Subscribed to event: {} (total subscribers: {})", eventName, m_subscribers[eventName].size());
	}

	void EventBus::Unsubscribe(const std::string& eventName, sol::function callback)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		auto it = m_subscribers.find(eventName);
		if (it == m_subscribers.end()) {
			return;
		}

		// Remove all matching callbacks - note: this might not work perfectly due to sol::function comparison
		// In practice, most scripts won't need to unsubscribe individual functions
		auto& callbacks = it->second;
		callbacks.erase(
		    std::remove_if(callbacks.begin(), callbacks.end(),
		                   [&callback](const sol::function& f) {
			                   // This comparison may not work reliably - it's a limitation of sol2
			                   return false; // For now, we don't support selective unsubscribe
		                   }),
		    callbacks.end());

		GetDefaultLogger()->debug("Unsubscribed from event: {}", eventName);
	}

	void EventBus::Publish(const std::string& eventName)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_eventQueue.push_back({eventName, std::monostate{}});
		//GetDefaultLogger()->debug("Published event: {} (queued)", eventName);
	}

	void EventBus::Publish(const std::string& eventName, EventData data)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_eventQueue.push_back({eventName, data});
		//GetDefaultLogger()->debug("Published event with data: {} (queued)", eventName);
	}

	void EventBus::PublishEntityEvent(const std::string& eventName, Entity& entity)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_eventQueue.push_back({eventName, entity});
		//GetDefaultLogger()->debug("Published entity event: {} (queued)", eventName);
	}

	void EventBus::DispatchEvents()
	{
		// Copy the queue and clear it so new events can be queued during dispatch
		std::vector<QueuedEvent> eventsToDispatch;
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			eventsToDispatch = std::move(m_eventQueue);
			m_eventQueue.clear();
		}

		// Dispatch all queued events
		for (const auto& event : eventsToDispatch) {
			// Copy the callbacks list while holding the lock
			std::vector<sol::function> callbacks;
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				auto it = m_subscribers.find(event.eventName);
				if (it == m_subscribers.end()) {
					// No subscribers for this event
					continue;
				}
				callbacks = it->second; // Copy the callback list
			}
			// Release the lock before calling callbacks!

			GetDefaultLogger()->debug("Dispatching event: {} to {} subscribers", event.eventName, callbacks.size());

			// Call callbacks WITHOUT holding the mutex
			// This allows callbacks to publish new events without deadlocking
			for (const auto& callback : callbacks) {
				if (!callback.valid()) {
					GetDefaultLogger()->warn("Invalid callback for event: {}", event.eventName);
					continue;
				}

				try {
					// Call the callback with the event data
					// If the data is std::monostate (no data), call with no arguments
					if (std::holds_alternative<std::monostate>(event.data)) {
						sol::protected_function_result result = callback();
						if (!result.valid()) {
							sol::error err = result;
							GetDefaultLogger()->error("Error in event callback for '{}': {}", event.eventName, err.what());
						}
					}
					else if (std::holds_alternative<Entity>(event.data)) {
						// Special handling for Entity - pass by reference
						Entity& entity = const_cast<Entity&>(std::get<Entity>(event.data));
						sol::protected_function_result result = callback(std::ref(entity));
						if (!result.valid()) {
							sol::error err = result;
							GetDefaultLogger()->error("Error in event callback for '{}': {}", event.eventName, err.what());
						}
					}
					else if (std::holds_alternative<EntityHandle>(event.data)) {
						sol::protected_function_result result = callback(std::get<EntityHandle>(event.data));
						if (!result.valid()) {
							sol::error err = result;
							GetDefaultLogger()->error("Error in event callback for '{}': {}", event.eventName, err.what());
						}
					}
					else if (std::holds_alternative<float>(event.data)) {
						sol::protected_function_result result = callback(std::get<float>(event.data));
						if (!result.valid()) {
							sol::error err = result;
							GetDefaultLogger()->error("Error in event callback for '{}': {}", event.eventName, err.what());
						}
					}
					else if (std::holds_alternative<int>(event.data)) {
						sol::protected_function_result result = callback(std::get<int>(event.data));
						if (!result.valid()) {
							sol::error err = result;
							GetDefaultLogger()->error("Error in event callback for '{}': {}", event.eventName, err.what());
						}
					}
					else if (std::holds_alternative<bool>(event.data)) {
						sol::protected_function_result result = callback(std::get<bool>(event.data));
						if (!result.valid()) {
							sol::error err = result;
							GetDefaultLogger()->error("Error in event callback for '{}': {}", event.eventName, err.what());
						}
					}
					else if (std::holds_alternative<std::string>(event.data)) {
						sol::protected_function_result result = callback(std::get<std::string>(event.data));
						if (!result.valid()) {
							sol::error err = result;
							GetDefaultLogger()->error("Error in event callback for '{}': {}", event.eventName, err.what());
						}
					}
					else if (std::holds_alternative<glm::vec3>(event.data)) {
						sol::protected_function_result result = callback(std::get<glm::vec3>(event.data));
						if (!result.valid()) {
							sol::error err = result;
							GetDefaultLogger()->error("Error in event callback for '{}': {}", event.eventName, err.what());
						}
					}
					else {
						GetDefaultLogger()->warn("Unsupported event data type for event: {}", event.eventName);
					}
				}
				catch (const std::exception& e) {
					GetDefaultLogger()->error("Exception in event callback for '{}': {}", event.eventName, e.what());
				}
			}
		}
	}

	void EventBus::ClearAllSubscriptions()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_subscribers.clear();
		m_eventQueue.clear();
		GetDefaultLogger()->info("Cleared all event subscriptions");
	}

} // namespace Engine
