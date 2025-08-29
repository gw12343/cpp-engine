//
// Created by gabe on 8/27/25.
//

#include "NavigationManager.h"
#include "components/impl/NavigationComponents.h"
#include "components/impl/TransformComponent.h"
#include "components/impl/ModelRendererComponent.h"

// Recast/Detour includes
#include <Recast.h>
#include <DetourNavMesh.h>
#include <DetourNavMeshQuery.h>
#include <DetourNavMeshBuilder.h>
#include <DetourCrowd.h>

#include <imgui.h>
#include <algorithm>
#include <cstring>
#include <cfloat>

namespace Engine {

	// Custom Recast context for logging
	class NavigationContext : public rcContext {
	  public:
		NavigationContext() = default;

	  protected:
		void doLog(const rcLogCategory category, const char* msg, const int len) override
		{
			GetNav().log->info("rc info: {}", msg);
			switch (category) {
				case RC_LOG_ERROR:
					GetNav().log->error("rc error: {}", msg);
					break;
				case RC_LOG_WARNING:
					GetNav().log->warn("rc warn: {}", msg);
					break;
				default:
					GetNav().log->info("rc info: {}", msg);
			}
			// Forward to your logging system if available
			// For now, just ignore or print to console
		}
	};

	NavigationModule::NavigationModule() : m_context(std::make_unique<NavigationContext>()), m_navMesh(nullptr), m_navQuery(nullptr), m_filter(nullptr), m_crowd(nullptr), m_currentScene(nullptr), m_crowdInitialized(false)
	{
	}

	NavigationModule::~NavigationModule()
	{
		onShutdown();
	}

	void NavigationModule::onInit()
	{
		// Initialize Detour navigation query
		m_navQuery = dtAllocNavMeshQuery();

		// Initialize query filter
		m_filter = new dtQueryFilter();
		m_filter->setIncludeFlags(0xFFFF);
		m_filter->setExcludeFlags(0);

		// Initialize crowd simulation
		m_crowd = dtAllocCrowd();
	}

	void NavigationModule::onGameStart()
	{
		// Navigation mesh will be built when needed
	}

	void NavigationModule::onUpdate(float dt)
	{
		if (!m_currentScene || !m_crowd) return;

		// Update crowd simulation
		UpdateCrowdAgents(dt);

		// Sync crowd agent positions with entity components
		SyncAgentComponents();
		OnInspectorRender();
	}

	void NavigationModule::onShutdown()
	{
		ClearNavMesh();

		if (m_crowd) {
			dtFreeCrowd(m_crowd);
			m_crowd = nullptr;
		}

		if (m_navQuery) {
			dtFreeNavMeshQuery(m_navQuery);
			m_navQuery = nullptr;
		}

		delete m_filter;
		m_filter = nullptr;
	}

	bool NavigationModule::BuildNavMesh(Scene* scene)
	{
		if (!scene) {
			log->warn("scene null");
			return false;
		}

		m_currentScene = scene;

		// Clear existing navmesh
		ClearNavMesh();

		// Collect geometry from scene
		GeometryData geometry;
		CollectGeometry(scene, geometry);

		if (geometry.vertices.empty()) {
			log->warn("geometry empty");
			return false;
		}

		// Calculate bounding box
		float bmin[3] = {FLT_MAX, FLT_MAX, FLT_MAX};
		float bmax[3] = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

		for (size_t i = 0; i < geometry.vertices.size(); i += 3) {
			bmin[0] = std::min(bmin[0], geometry.vertices[i]);
			bmin[1] = std::min(bmin[1], geometry.vertices[i + 1]);
			bmin[2] = std::min(bmin[2], geometry.vertices[i + 2]);
			bmax[0] = std::max(bmax[0], geometry.vertices[i]);
			bmax[1] = std::max(bmax[1], geometry.vertices[i + 1]);
			bmax[2] = std::max(bmax[2], geometry.vertices[i + 2]);
		}

		// Build heightfield
		rcHeightfield* heightfield = rcAllocHeightfield();
		if (!rcCreateHeightfield(m_context.get(), *heightfield, int((bmax[0] - bmin[0]) / m_config.cellSize) + 1, int((bmax[2] - bmin[2]) / m_config.cellSize) + 1, bmin, bmax, m_config.cellSize, m_config.cellHeight)) {
			rcFreeHeightField(heightfield);
			log->warn("failed to create heightfield");
			return false;
		}

		// Rasterize triangles
		std::vector<unsigned char> triangleAreas(geometry.indices.size() / 3);
		rcMarkWalkableTriangles(
		    m_context.get(), m_config.agentMaxSlope, geometry.vertices.data(), static_cast<int>(geometry.vertices.size() / 3), geometry.indices.data(), static_cast<int>(geometry.indices.size() / 3), triangleAreas.data());

		if (!rcRasterizeTriangles(m_context.get(),
		                          geometry.vertices.data(),
		                          static_cast<int>(geometry.vertices.size() / 3),
		                          geometry.indices.data(),
		                          triangleAreas.data(),
		                          static_cast<int>(geometry.indices.size() / 3),
		                          *heightfield,
		                          m_config.agentMaxClimb)) {
			rcFreeHeightField(heightfield);
			log->warn("failed to rasterize triangles");
			return false;
		}

		// Filter walkable surfaces
		rcFilterLowHangingWalkableObstacles(m_context.get(), m_config.agentMaxClimb, *heightfield);
		rcFilterLedgeSpans(m_context.get(), m_config.agentHeight, m_config.agentMaxClimb, *heightfield);
		rcFilterWalkableLowHeightSpans(m_context.get(), m_config.agentHeight, *heightfield);

		// Create compact heightfield
		rcCompactHeightfield* compactHeightfield = rcAllocCompactHeightfield();
		if (!rcBuildCompactHeightfield(m_context.get(), m_config.agentHeight, m_config.agentMaxClimb, *heightfield, *compactHeightfield)) {
			rcFreeHeightField(heightfield);
			rcFreeCompactHeightfield(compactHeightfield);
			log->warn("failed to compact heightfield");
			return false;
		}

		rcFreeHeightField(heightfield);

		// Build distance field and regions
		if (!rcBuildDistanceField(m_context.get(), *compactHeightfield)) {
			rcFreeCompactHeightfield(compactHeightfield);
			log->warn("failed to build distance field");
			return false;
		}

		if (!rcBuildRegions(m_context.get(), *compactHeightfield, 0, m_config.regionMinSize, m_config.regionMergeSize)) {
			rcFreeCompactHeightfield(compactHeightfield);
			log->warn("failed to build regions");
			return false;
		}

		// Build contours
		rcContourSet* contours = rcAllocContourSet();
		if (!rcBuildContours(m_context.get(), *compactHeightfield, m_config.maxSimplificationError, m_config.maxEdgeLength, *contours)) {
			rcFreeCompactHeightfield(compactHeightfield);
			rcFreeContourSet(contours);
			log->warn("failed to build contors");
			return false;
		}

		// Build polygon mesh
		rcPolyMesh* polyMesh = rcAllocPolyMesh();
		if (!rcBuildPolyMesh(m_context.get(), *contours, m_config.minVertsPerPoly, *polyMesh)) {
			rcFreeCompactHeightfield(compactHeightfield);
			rcFreeContourSet(contours);
			rcFreePolyMesh(polyMesh);
			log->warn("failed to build poly mesh");
			return false;
		}

		// Build detail mesh for more accurate height queries
		rcPolyMeshDetail* detailMesh = rcAllocPolyMeshDetail();
		if (!rcBuildPolyMeshDetail(m_context.get(), *polyMesh, *compactHeightfield, m_config.detailSampleDist, m_config.detailSampleMaxError, *detailMesh)) {
			rcFreeCompactHeightfield(compactHeightfield);
			rcFreeContourSet(contours);
			rcFreePolyMesh(polyMesh);
			rcFreePolyMeshDetail(detailMesh);
			log->warn("failed to build detail mesh");
			return false;
		}

		rcFreeCompactHeightfield(compactHeightfield);
		rcFreeContourSet(contours);

		// Create Detour navigation mesh
		dtNavMeshCreateParams params;
		memset(&params, 0, sizeof(params));
		params.verts            = polyMesh->verts;
		params.vertCount        = polyMesh->nverts;
		params.polys            = polyMesh->polys;
		params.polyAreas        = polyMesh->areas;
		params.polyFlags        = polyMesh->flags;
		params.polyCount        = polyMesh->npolys;
		params.nvp              = polyMesh->nvp;
		params.detailMeshes     = detailMesh->meshes;
		params.detailVerts      = detailMesh->verts;
		params.detailVertsCount = detailMesh->nverts;
		params.detailTris       = detailMesh->tris;
		params.detailTriCount   = detailMesh->ntris;
		params.walkableHeight   = m_config.agentHeight;
		params.walkableRadius   = m_config.agentRadius;
		params.walkableClimb    = m_config.agentMaxClimb;
		rcVcopy(params.bmin, polyMesh->bmin);
		rcVcopy(params.bmax, polyMesh->bmax);
		params.cs          = m_config.cellSize;
		params.ch          = m_config.cellHeight;
		params.buildBvTree = true;

		unsigned char* navData     = nullptr;
		int            navDataSize = 0;

		// Assign flags based on area
		for (int i = 0; i < polyMesh->npolys; ++i) {
			if (polyMesh->areas[i] == RC_WALKABLE_AREA)
				polyMesh->flags[i] = 1; // walkable
			else
				polyMesh->flags[i] = 0; // not walkable
		}
		

		if (!dtCreateNavMeshData(&params, &navData, &navDataSize)) {
			rcFreePolyMesh(polyMesh);
			rcFreePolyMeshDetail(detailMesh);
			log->warn("failed to build nav mesh detail");
			return false;
		}

		rcFreePolyMesh(polyMesh);
		rcFreePolyMeshDetail(detailMesh);

		// Initialize Detour navmesh
		m_navMesh = dtAllocNavMesh();
		if (dtStatusFailed(m_navMesh->init(navData, navDataSize, DT_TILE_FREE_DATA))) {
			dtFree(navData);
			dtFreeNavMesh(m_navMesh);
			log->warn("failed to init detour navmesh");
			m_navMesh = nullptr;
			return false;
		}

		// Initialize navmesh query
		if (dtStatusFailed(m_navQuery->init(m_navMesh, 2048))) {
			log->warn("failed to init navmesh query");
			return false;
		}

		// Initialize crowd
		if (!m_crowd->init(1000, m_config.agentRadius, m_navMesh)) {
			log->warn("failed to init crowd");
			return false;
		}
		m_crowdInitialized = true;
		return true;
	}

	void NavigationModule::ClearNavMesh()
	{
		if (m_navMesh) {
			dtFreeNavMesh(m_navMesh);
			m_navMesh = nullptr;
		}
		m_crowdInitialized = false;
	}

	void NavigationModule::CollectGeometry(Scene* scene, GeometryData& geometry)
	{
		auto registry = scene->GetRegistry();
		auto view     = registry->view<Components::NavMeshGeometry, Components::Transform>();

		for (auto entity : view) {
			Entity ent(entity, scene);
			AddEntityGeometry(ent, geometry);
		}
	}

	void NavigationModule::AddEntityGeometry(Entity entity, GeometryData& geometry)
	{
		if (!entity.HasComponent<Components::ModelRenderer>()) return;

		auto& transform    = entity.GetComponent<Components::Transform>();
		auto& meshRenderer = entity.GetComponent<Components::ModelRenderer>();
		auto& navGeometry  = entity.GetComponent<Components::NavMeshGeometry>();

		// TODO: Replace this with your actual mesh system
		// This is a placeholder - you need to adapt this to your mesh format

		// Example: Create a simple quad for testing
		if (navGeometry.isWalkable) {
			size_t baseIndex = geometry.vertices.size() / 3;

			// Simple quad vertices (you'll replace this with actual mesh data)
			std::vector<glm::vec3> localVertices = {glm::vec3(-1.0f, 0.0f, -1.0f), glm::vec3(1.0f, 0.0f, -1.0f), glm::vec3(1.0f, 0.0f, 1.0f), glm::vec3(-1.0f, 0.0f, 1.0f)};

			std::vector<uint32_t> localIndices = {0, 1, 2, 0, 2, 3};

			// Transform vertices to world space
			glm::mat4 worldMatrix = transform.GetMatrix();
			for (const auto& vertex : localVertices) {
				glm::vec4 worldPos = worldMatrix * glm::vec4(vertex, 1.0f);
				geometry.vertices.push_back(worldPos.x);
				geometry.vertices.push_back(worldPos.y);
				geometry.vertices.push_back(worldPos.z);
			}

			// Add indices with base offset
			for (size_t i = 0; i < localIndices.size(); i += 3) {
				geometry.indices.push_back(baseIndex + localIndices[i]);
				geometry.indices.push_back(baseIndex + localIndices[i + 1]);
				geometry.indices.push_back(baseIndex + localIndices[i + 2]);
				geometry.areas.push_back(navGeometry.area);
			}
		}
	}

	bool NavigationModule::FindPath(const glm::vec3& start, const glm::vec3& end, std::vector<glm::vec3>& path)
	{
		if (!m_navMesh || !m_navQuery) return false;

		dtPolyRef   startRef, endRef;
		float       startNearestPt[3], endNearestPt[3];
		const float extents[3] = {2.0f, 4.0f, 2.0f};

		// Find nearest polys to start and end positions
		m_navQuery->findNearestPoly(&start.x, extents, m_filter, &startRef, startNearestPt);
		m_navQuery->findNearestPoly(&end.x, extents, m_filter, &endRef, endNearestPt);

		if (!startRef || !endRef) return false;

		// Find path
		dtPolyRef pathPolys[256];
		int       pathCount = 0;

		m_navQuery->findPath(startRef, endRef, startNearestPt, endNearestPt, m_filter, pathPolys, &pathCount, 256);

		if (pathCount == 0) return false;

		// Get straight path
		float         straightPath[256 * 3];
		unsigned char straightPathFlags[256];
		dtPolyRef     straightPathPolys[256];
		int           straightPathCount = 0;

		m_navQuery->findStraightPath(startNearestPt, endNearestPt, pathPolys, pathCount, straightPath, straightPathFlags, straightPathPolys, &straightPathCount, 256);

		// Convert to glm::vec3
		path.clear();
		path.reserve(straightPathCount);

		for (int i = 0; i < straightPathCount; ++i) {
			path.emplace_back(straightPath[i * 3], straightPath[i * 3 + 1], straightPath[i * 3 + 2]);
		}

		return true;
	}

	bool NavigationModule::SetAgentTarget(Entity agent, const glm::vec3& target)
	{
		if (!agent.HasComponent<Components::NavAgent>()) return false;

		auto& navAgent = agent.GetComponent<Components::NavAgent>();

		// Find path to target
		std::vector<glm::vec3> path;
		auto&                  transform = agent.GetComponent<Components::Transform>();

		if (FindPath(transform.position, target, path)) {
			navAgent.path           = path;
			navAgent.targetPosition = target;
			navAgent.hasPath        = true;
			navAgent.isMoving       = true;

			// Update crowd agent if exists
			if (navAgent.crowdAgentId >= 0 && m_crowd && m_navQuery) {
				// Find the polygon reference for the target position
				dtPolyRef   targetRef;
				float       nearestPt[3];
				const float extents[3] = {2.0f, 4.0f, 2.0f};

				dtStatus status = m_navQuery->findNearestPoly(&target.x, extents, m_filter, &targetRef, nearestPt);

				if (dtStatusSucceed(status) && targetRef != 0) {
					m_crowd->requestMoveTarget(navAgent.crowdAgentId, targetRef, nearestPt);
				}
			}

			return true;
		}

		return false;
	}

	int NavigationModule::AddCrowdAgent(Entity entity)
	{
		if (!entity.HasComponent<Components::NavAgent>() || !m_crowd) return -1;

		auto& navAgent  = entity.GetComponent<Components::NavAgent>();
		auto& transform = entity.GetComponent<Components::Transform>();

		dtCrowdAgentParams params;
		memset(&params, 0, sizeof(params));
		params.radius                = navAgent.radius;
		params.height                = navAgent.height;
		params.maxAcceleration       = 8.0f;
		params.maxSpeed              = navAgent.maxSpeed;
		params.collisionQueryRange   = params.radius * 12.0f;
		params.pathOptimizationRange = params.radius * 30.0f;
		params.updateFlags           = DT_CROWD_ANTICIPATE_TURNS | DT_CROWD_OPTIMIZE_VIS | DT_CROWD_OPTIMIZE_TOPO | DT_CROWD_OBSTACLE_AVOIDANCE;
		params.obstacleAvoidanceType = 3;
		params.separationWeight      = 2.0f;

		int agentId = m_crowd->addAgent(&transform.position.x, &params);
		if (agentId >= 0) {
			navAgent.crowdAgentId = agentId;
		}

		return agentId;
	}

	void NavigationModule::RemoveCrowdAgent(int agentId)
	{
		if (m_crowd && agentId >= 0) {
			m_crowd->removeAgent(agentId);
		}
	}

	glm::vec3 NavigationModule::GetRandomPoint()
	{
		if (!m_navMesh || !m_navQuery) return glm::vec3(0.0f);

		dtPolyRef randomRef;
		float     randomPt[3];

		dtStatus status = m_navQuery->findRandomPoint(
		    m_filter, []() { return static_cast<float>(rand()) / RAND_MAX; }, &randomRef, randomPt);

		if (dtStatusSucceed(status)) {
			return glm::vec3(randomPt[0], randomPt[1], randomPt[2]);
		}

		return glm::vec3(0.0f);
	}

	glm::vec3 NavigationModule::GetNearestPoint(const glm::vec3& position)
	{
		if (!m_navMesh || !m_navQuery) return position;

		dtPolyRef   nearestRef;
		float       nearestPt[3];
		const float extents[3] = {2.0f, 4.0f, 2.0f};

		dtStatus status = m_navQuery->findNearestPoly(&position.x, extents, m_filter, &nearestRef, nearestPt);

		if (dtStatusSucceed(status) && nearestRef != 0) {
			return glm::vec3(nearestPt[0], nearestPt[1], nearestPt[2]);
		}

		return position;
	}

	void NavigationModule::UpdateCrowdAgents(float dt)
	{
		// Only update if crowd is properly initialized
		if (!m_crowd || !m_crowdInitialized) return;

		m_crowd->update(dt, nullptr);
	}

	void NavigationModule::SyncAgentComponents()
	{
		if (!m_currentScene || !m_crowd) return;

		auto registry = m_currentScene->GetRegistry();
		auto view     = registry->view<Components::NavAgent, Components::Transform>();

		for (auto entity : view) {
			Entity ent(entity, m_currentScene);
			auto&  navAgent  = ent.GetComponent<Components::NavAgent>();
			auto&  transform = ent.GetComponent<Components::Transform>();

			if (navAgent.crowdAgentId >= 0) {
				const dtCrowdAgent* agent = m_crowd->getAgent(navAgent.crowdAgentId);
				if (agent && agent->active) {
					transform.position = glm::vec3(agent->npos[0], agent->npos[1], agent->npos[2]);
					navAgent.isMoving  = (agent->vel[0] != 0 || agent->vel[2] != 0);
				}
			}
		}
	}

	void NavigationModule::OnInspectorRender()
	{
		ImGui::Begin("Navmesh Inspector");
		if (ImGui::CollapsingHeader("Navigation System")) {
			ImGui::Checkbox("Show Debug Mesh", &m_showDebugMesh);
			ImGui::Checkbox("Show Agent Paths", &m_showAgentPaths);

			if (ImGui::Button("Build NavMesh")) {
				if (m_currentScene) {
					bool success = BuildNavMesh(m_currentScene);
					ImGui::Text("Build result: %s", success ? "Success" : "Failed");
				}
			}

			ImGui::SameLine();
			if (ImGui::Button("Clear NavMesh")) {
				ClearNavMesh();
			}

			// Configuration
			if (ImGui::CollapsingHeader("NavMesh Config")) {
				ImGui::SliderFloat("Cell Size", &m_config.cellSize, 0.1f, 1.0f);
				ImGui::SliderFloat("Cell Height", &m_config.cellHeight, 0.1f, 1.0f);
				ImGui::SliderFloat("Agent Height", &m_config.agentHeight, 0.5f, 5.0f);
				ImGui::SliderFloat("Agent Radius", &m_config.agentRadius, 0.1f, 2.0f);
				ImGui::SliderFloat("Agent Max Climb", &m_config.agentMaxClimb, 0.1f, 2.0f);
				ImGui::SliderFloat("Agent Max Slope", &m_config.agentMaxSlope, 1.0f, 90.0f);
			}
		}
		ImGui::End();
	}

} // namespace Engine