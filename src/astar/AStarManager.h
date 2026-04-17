#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "AStarNode.h"

class Surface;

class AStarManager {
public:
    static AStarManager& Instance() {
        static AStarManager instance;
        return instance;
    }

    void BuildGraph(Surface* surface, float connectionRadius = 1.5f);

    std::vector<glm::vec3> FindPath(const glm::vec3& start, const glm::vec3& end);

    AStarNode* FindNearestNode(const glm::vec3& position);

    const std::vector<AStarNode*>& GetAllNodes() const { return m_Nodes; }

    void Clear();

private:
    AStarManager() = default;
    ~AStarManager() { Clear(); }

    std::vector<AStarNode*> m_Nodes;
    bool m_GraphBuilt = false;
};