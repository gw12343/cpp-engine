#pragma once

#include <glm/glm.hpp>
#include <vector>
#include "core/Entity.h"
#include "cereal/cereal.hpp"

namespace Engine {
	namespace Components {
		// Component for marking entities as navigation mesh geometry
		class NavMeshGeometry : public Component {
		  public:
			bool isWalkable = true;
			int  area       = 0; // Navigation area type (0 = walkable)
			void OnAdded(Entity& entity) override {}
			void OnRemoved(Entity& entity) override{};
			void RenderInspector(Entity& entity) override;

			template <class Archive>
			void serialize(Archive& ar)
			{
				ar(CEREAL_NVP(isWalkable), CEREAL_NVP(area));
			}
		};

		// Component for navigation agents
		class NavAgent : public Component {
		  public:
			float                  radius         = 0.5f;
			float                  height         = 2.0f;
			float                  maxSpeed       = 3.5f;
			glm::vec3              targetPosition = glm::vec3(0.0f);
			std::vector<glm::vec3> path;
			bool                   hasPath      = false;
			bool                   isMoving     = false;
			int                    crowdAgentId = -1;


			float  speed             = 3.0f;
			size_t currentWaypoint   = 0;
			float  waypointTolerance = 0.2f;


			template <class Archive>
			void serialize(Archive& ar)
			{
				ar(CEREAL_NVP(radius), CEREAL_NVP(height), CEREAL_NVP(maxSpeed));
			}

			void OnAdded(Entity& entity) override {}
			void OnRemoved(Entity& entity) override{};
			void RenderInspector(Entity& entity) override;
		};
	} // namespace Components
} // namespace Engine