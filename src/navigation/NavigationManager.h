// NavigationModule.h
#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include "core/module/Module.h"
#include "core/Entity.h"
#include "core/Scene.h"

// Forward declarations for Recast/Detour
class rcContext;
class dtNavMesh;
class dtNavMeshQuery;
class dtQueryFilter;
class dtCrowd;

namespace Engine {

	// Navigation build configuration
	struct NavMeshConfig {
		float cellSize               = 0.3f;
		float cellHeight             = 0.2f;
		float agentHeight            = 2.0f;
		float agentRadius            = 0.6f;
		float agentMaxClimb          = 0.9f;
		float agentMaxSlope          = 45.0f;
		int   regionMinSize          = 8;
		int   regionMergeSize        = 20;
		int   maxEdgeLength          = 12;
		float maxSimplificationError = 1.3f;
		int   minVertsPerPoly        = 3;
		float detailSampleDist       = 6.0f;
		float detailSampleMaxError   = 1.0f;
	};

	class NavigationModule : public Module {
	  public:
		NavigationModule();
		~NavigationModule() override;

		void        onInit() override;
		void        onGameStart() override;
		void        onUpdate(float dt) override;
		void        onShutdown() override;
		void        setLuaBindings() override {}
		std::string name() const override { return "NavigationModule"; }

		// Navigation mesh management
		bool BuildNavMesh(Scene* scene);
		void ClearNavMesh();

		// Pathfinding
		bool FindPath(const glm::vec3& start, const glm::vec3& end, std::vector<glm::vec3>& path);
		bool SetAgentTarget(Entity agent, const glm::vec3& target);

		// Agent management
		int  AddCrowdAgent(Entity entity);
		void RemoveCrowdAgent(int agentId);

		// Utility functions
		glm::vec3 GetRandomPoint();
		glm::vec3 GetNearestPoint(const glm::vec3& position);

		// Debug
		void OnInspectorRender();

		// Scene reference
		Scene* m_currentScene;

		dtNavMesh* m_navMesh;

		void Render();

	  private:
		bool m_crowdInitialized;
		struct GeometryData {
			std::vector<float> vertices;
			std::vector<int>   indices;
			std::vector<int>   areas;
		};

		// Recast/Detour objects
		std::unique_ptr<rcContext> m_context;
		dtNavMeshQuery*            m_navQuery;
		dtQueryFilter*             m_filter;
		dtCrowd*                   m_crowd;

		// Configuration
		NavMeshConfig m_config;

		// Debug flags
		bool m_showDebugMesh  = true;
		bool m_showAgentPaths = true;

		// Helper methods
		void CollectGeometry(Scene* scene, GeometryData& geometry);
		void AddEntityGeometry(Entity entity, GeometryData& geometry);
		void UpdateCrowdAgents(float dt);
		void SyncAgentComponents();
	};

} // namespace Engine
