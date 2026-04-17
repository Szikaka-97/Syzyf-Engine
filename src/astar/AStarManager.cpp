// AStarManager.cpp
#include "AStarManager.h"
#include "Surface.h"
#include <algorithm>
#include <unordered_map>
#include <queue>
#include <spdlog/spdlog.h>

void AStarManager::BuildGraph(Surface* surface, float connectionRadius) {
    Clear();
    if (!surface) {
        spdlog::error("AStarManager::BuildGraph - Surface is null");
        return;
    }

    const auto& points = surface->GetWalkablePoints();
    if (points.empty()) {
        spdlog::warn("AStarManager::BuildGraph - No walkable points on surface");
        return;
    }

    m_Nodes.reserve(points.size());
    for (const auto& pos : points) {
        m_Nodes.push_back(new AStarNode(pos));
    }

    const float radiusSq = connectionRadius * connectionRadius;
    for (size_t i = 0; i < m_Nodes.size(); ++i) {
        for (size_t j = i + 1; j < m_Nodes.size(); ++j) {
            float distSq = glm::distance(m_Nodes[i]->position, m_Nodes[j]->position);
            if (distSq <= radiusSq) {
                m_Nodes[i]->connections.push_back(m_Nodes[j]);
                m_Nodes[j]->connections.push_back(m_Nodes[i]);
            }
        }
    }

    m_GraphBuilt = true;
    spdlog::info("AStarManager: Graph built with {} nodes", m_Nodes.size());
}

void AStarManager::Clear() {
    for (auto node : m_Nodes) {
        delete node;
    }
    m_Nodes.clear();
    m_GraphBuilt = false;
}

AStarNode* AStarManager::FindNearestNode(const glm::vec3& position) {
    if (m_Nodes.empty()) return nullptr;

    AStarNode* nearest = nullptr;
    float minDistSq = std::numeric_limits<float>::max();
    for (auto node : m_Nodes) {
        float distSq = glm::distance(position, node->position);
        if (distSq < minDistSq) {
            minDistSq = distSq;
            nearest = node;
        }
    }
    return nearest;
}

std::vector<glm::vec3> AStarManager::FindPath(const glm::vec3& start, const glm::vec3& end) {
    if (!m_GraphBuilt || m_Nodes.empty()) {
        spdlog::warn("AStarManager::FindPath - Graph not built");
        return {};
    }

    AStarNode* startNode = FindNearestNode(start);
    AStarNode* endNode = FindNearestNode(end);
    if (!startNode || !endNode) {
        spdlog::warn("AStarManager::FindPath - Start or end node not found");
        return {};
    }

    for (auto node : m_Nodes) {
        node->ResetPathfindingData();
    }

    std::vector<AStarNode*> openSet;
    startNode->gScore = 0.0f;
    startNode->hScore = glm::distance(startNode->position, endNode->position);
    openSet.push_back(startNode);

    while (!openSet.empty()) {
        auto it = std::min_element(openSet.begin(), openSet.end(),
            [](AStarNode* a, AStarNode* b) { return a->FScore() < b->FScore(); });
        AStarNode* current = *it;
        openSet.erase(it);

        if (current == endNode) {
            std::vector<glm::vec3> path;
            while (current != startNode) {
                path.push_back(current->position);
                current = current->cameFrom;
            }
            path.push_back(startNode->position);
            std::reverse(path.begin(), path.end());
            return path;
        }

        for (AStarNode* neighbor : current->connections) {
            float tentativeG = current->gScore + glm::distance(current->position, neighbor->position);
            if (tentativeG < neighbor->gScore) {
                neighbor->cameFrom = current;
                neighbor->gScore = tentativeG;
                neighbor->hScore = glm::distance(neighbor->position, endNode->position);
                if (std::find(openSet.begin(), openSet.end(), neighbor) == openSet.end()) {
                    openSet.push_back(neighbor);
                }
            }
        }
    }

    spdlog::warn("AStarManager::FindPath - No path found");
    return {};
}