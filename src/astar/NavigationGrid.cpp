#include "NavigationGrid.h"
#include "Surface.h"
#include <algorithm>         
#include <limits>           
#include <random>           
#include <glm/glm.hpp>       
#include <glm/gtc/constants.hpp>
#include <spdlog/spdlog.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

void NavigationGrid::Build(Surface* surface, float cellSize, float maxSlopeDeg) {
        if (!surface) return;
    m_surface = surface;
    m_cellSize = cellSize;
    m_maxSlopeCos = glm::cos(glm::radians(maxSlopeDeg));

    glm::vec3 minBound(-150.0f, 0.0f, -150.0f);
    glm::vec3 maxBound( 150.0f, 0.0f,  150.0f);

    m_gridSize.x = static_cast<int>((maxBound.x - minBound.x) / m_cellSize) + 1;
    m_gridSize.y = static_cast<int>((maxBound.z - minBound.z) / m_cellSize) + 1;
    m_origin = minBound;

    m_nodes.resize(m_gridSize.x * m_gridSize.y);

    for (int y = 0; y < m_gridSize.y; ++y) {
        for (int x = 0; x < m_gridSize.x; ++x) {
            int idx = GetIndex(x, y);
            NavGridNode& node = m_nodes[idx];
            node.gridPos = {x, y};
            node.worldPos = m_origin + glm::vec3((x + 0.5f) * m_cellSize, 0.0f, (y + 0.5f) * m_cellSize);
            float groundY = m_surface->GetGroundHeight(node.worldPos.x, node.worldPos.z);
            node.worldPos.y = groundY;
            
            //node.walkable = m_surface->IsOnSurface(glm::vec3(node.worldPos.x, groundY + 0.1f, node.worldPos.z));
            node.walkable = m_surface->IsOnSurface(glm::vec3(node.worldPos.x, groundY + 0.1f, node.worldPos.z))
                && !IsBlockedByObstacle(glm::vec3(node.worldPos.x, groundY + 0.1f, node.worldPos.z));
        }
    }

    ConnectNeighbors();
    m_built = true;

    size_t walkableCount = std::count_if(m_nodes.begin(), m_nodes.end(),
                                         [](auto& n){ return n.walkable; });
    spdlog::info("NavigationGrid: {}x{} cells, {} walkable nodes",
                 m_gridSize.x, m_gridSize.y, walkableCount);
}

void NavigationGrid::ConnectNeighbors() {
    const glm::ivec2 dirs[8] = {
        {1,0},{-1,0},{0,1},{0,-1},
        {1,1},{1,-1},{-1,1},{-1,-1}
    };
    for (int y = 0; y < m_gridSize.y; ++y) {
        for (int x = 0; x < m_gridSize.x; ++x) {
            NavGridNode& node = m_nodes[GetIndex(x, y)];
            if (!node.walkable) continue;
            for (auto d : dirs) {
                int nx = x + d.x, ny = y + d.y;
                if (!IsInsideGrid(nx, ny)) continue;
                NavGridNode& nb = m_nodes[GetIndex(nx, ny)];
                if (!nb.walkable) continue;
                if (d.x != 0 && d.y != 0) {
                    if (!m_nodes[GetIndex(x + d.x, y)].walkable ||
                        !m_nodes[GetIndex(x, y + d.y)].walkable)
                        continue;
                }
                if (glm::abs(node.worldPos.y - nb.worldPos.y) > 1.0f) continue;
                node.neighbors.push_back(&nb);
            }
        }
    }
}


glm::ivec2 NavigationGrid::WorldToGrid(const glm::vec3& worldPos) const {
    glm::vec3 local = worldPos - m_origin;
    int x = static_cast<int>(glm::floor(local.x / m_cellSize));
    int y = static_cast<int>(glm::floor(local.z / m_cellSize));
    return { x, y };
}

bool NavigationGrid::IsWalkableAt(const glm::vec3& point) {
    if (!m_surface) return false;
    float groundY = m_surface->GetGroundHeight(point.x, point.z);
    if (std::abs(point.y - groundY) > 0.5f) return false;
    return true;
}

NavGridNode* NavigationGrid::GetNodeFromWorld(const glm::vec3& pos) {
    glm::ivec2 gridPos = WorldToGrid(pos);
    if (!IsInsideGrid(gridPos.x, gridPos.y)) return nullptr;
    NavGridNode* node = &m_nodes[GetIndex(gridPos.x, gridPos.y)];
    return node->walkable ? node : nullptr;
}

std::vector<glm::vec3> NavigationGrid::FindPath(const glm::vec3& start, const glm::vec3& end) {
    if (!m_built) return {};

    NavGridNode* startNode = GetNodeFromWorld(start);
    NavGridNode* endNode   = GetNodeFromWorld(end);
    if (!startNode || !endNode) {
        spdlog::warn("NavigationGrid::FindPath - start or end not walkable");
        return {};
    }

    for (auto& node : m_nodes) {
        node.Reset();
    }

    std::vector<NavGridNode*> openSet;
    startNode->gScore = 0.0f;
    startNode->hScore = glm::distance(startNode->worldPos, endNode->worldPos);
    openSet.push_back(startNode);

    while (!openSet.empty()) {
        auto it = std::min_element(openSet.begin(), openSet.end(),
            [](NavGridNode* a, NavGridNode* b) { return a->FScore() < b->FScore(); });
        NavGridNode* current = *it;
        openSet.erase(it);

        if (current == endNode) {
            std::vector<glm::vec3> path;
            while (current != startNode) {
                path.push_back(current->worldPos);
                current = current->cameFrom;
            }
            path.push_back(startNode->worldPos);
            std::reverse(path.begin(), path.end());
            return path;
        }

        for (NavGridNode* neighbor : current->neighbors) {
            float tentativeG = current->gScore + glm::distance(current->worldPos, neighbor->worldPos);
            if (tentativeG < neighbor->gScore) {
                neighbor->cameFrom = current;
                neighbor->gScore = tentativeG;
                neighbor->hScore = glm::distance(neighbor->worldPos, endNode->worldPos);
                if (std::find(openSet.begin(), openSet.end(), neighbor) == openSet.end()) {
                    openSet.push_back(neighbor);
                }
            }
        }
    }

    spdlog::warn("NavigationGrid::FindPath - no path found");
    return {};
}

bool NavigationGrid::IsBlockedByObstacle(const glm::vec3& point) const {
    auto* scene = m_surface ? m_surface->GetScene() : nullptr;
    if (!scene) return false;
    auto* physics = scene->GetComponent<Physics::System>();
    if (!physics) return false;

    JPH::ShapeRefC shape = new JPH::SphereShape(0.5f);
    glm::vec3 origin = point + glm::vec3(0.0f, 0.5f, 0.0f);
    glm::vec3 direction(0.0f, -1.0f, 0.0f);
    float maxDist = 1.0f;

    std::vector<SceneNode*> hits = physics->CastShape(origin, direction * maxDist, shape, {}, {}, {});
    for (SceneNode* hit : hits) {
        if (hit && hit->GetName() != "Floor") 
            return true;
    }
    return false;
}

glm::vec3 NavigationGrid::GetRandomWalkablePosition( glm::vec3 point, float radius) const {
    if (!m_built || m_nodes.empty()) {
        spdlog::warn("NavigationGrid::GetRandomWalkablePosition - grid not built");
         return point;
    }

    glm::ivec2 center = WorldToGrid(point);
    int cellRadius = static_cast<int>(std::ceil(radius / m_cellSize));
    
    std::vector<const NavGridNode*> candidates;

    for (int dy = -cellRadius; dy <= cellRadius; ++dy) {
        for (int dx = -cellRadius; dx <= cellRadius; ++dx) {
            int gx = center.x + dx;
            int gy = center.y + dy;
            if (!IsInsideGrid(gx, gy)) continue;
            
            const NavGridNode& node = m_nodes[GetIndex(gx, gy)];
            if (!node.walkable) continue;
            
            float dxf = node.worldPos.x - point.x;
            float dzf = node.worldPos.z - point.z;
            float dist = std::sqrt(dxf * dxf + dzf * dzf);
            
            if (dist <= radius) {
                candidates.push_back(&node);
            }
        }
    }

    if (candidates.empty()) {
        spdlog::warn("NavigationGrid: No walkable cells within radius {:.2f}", radius);
        return point;
    }

    static std::mt19937 gen(static_cast<unsigned>(
        std::chrono::steady_clock::now().time_since_epoch().count()));
    std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
    const NavGridNode* selected = candidates[dist(gen)];

    return selected->worldPos;
}