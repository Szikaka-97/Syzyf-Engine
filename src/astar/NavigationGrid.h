#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "NavGridNode.h"
#include "GameObject.h"

class Surface;

class NavigationGrid: public GameObject {
public:
    void Build(Surface* surface, float cellSize = 2.0f, float maxSlopeDeg = 45.0f);

    std::vector<glm::vec3> FindPath(const glm::vec3& start, const glm::vec3& end);
   glm::vec3 GetRandomWalkablePosition( glm::vec3 point, float radius) const;
    NavGridNode* GetNodeFromWorld(const glm::vec3& pos);
    bool IsBuilt() const { return m_built; }

private:
    Surface* m_surface = nullptr;
    float m_cellSize = 2.0f;
    float m_maxSlopeCos = 0.707f; // cos(45)
    bool m_built = false;

    std::vector<NavGridNode> m_nodes;
    glm::ivec2 m_gridSize;
    glm::vec3 m_origin;

    glm::ivec2 WorldToGrid(const glm::vec3& worldPos) const;
    int GetIndex(int x, int y) const { return y * m_gridSize.x + x; }
    bool IsInsideGrid(int x, int y) const {
        return x >= 0 && x < m_gridSize.x && y >= 0 && y < m_gridSize.y;
    }
    void ConnectNeighbors();
    bool IsWalkableAt(const glm::vec3& point);
};