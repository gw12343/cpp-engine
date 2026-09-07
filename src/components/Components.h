#pragma once

namespace Engine {
	class Entity;

	namespace Components {

		void RegisterAllComponentBindings();

		class Component {
		  public:
			Component()          = default;
			virtual ~Component() = default;

			virtual void OnAdded(Entity& entity)   = 0;
			virtual void OnRemoved(Entity& entity) = 0;

			virtual void RenderInspector(Entity& entity) {}

			static void AddBindings() {}
		};

	} // namespace Components
} // namespace Engine
