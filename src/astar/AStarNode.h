// AStarNode.h
#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <limits>

class AStarNode {
public:
    glm::vec3 position;
    std::vector<AStarNode*> connections;

    float gScore;
    float hScore;
    AStarNode* cameFrom;

    AStarNode(const glm::vec3& pos) 
        : position(pos), gScore(0.0f), hScore(0.0f), cameFrom(nullptr) {}

    float FScore() const { return gScore + hScore; }

    void ResetPathfindingData() ;
};