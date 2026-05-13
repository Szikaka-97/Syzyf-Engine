#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <limits>

struct NavGridNode {
    glm::ivec2 gridPos;
    glm::vec3 worldPos;
    bool walkable = false;
    std::vector<NavGridNode*> neighbors;

    // Dane tymczasowe dla A*
    float gScore = 0.0f;
    float hScore = 0.0f;
    NavGridNode* cameFrom = nullptr;

    float FScore() const { return gScore + hScore; }
    void Reset() {
        gScore = std::numeric_limits<float>::max();
        hScore = 0.0f;
        cameFrom = nullptr;
    }
};