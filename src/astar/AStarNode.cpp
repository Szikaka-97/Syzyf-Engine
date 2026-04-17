#include "AStarNode.h"

void AStarNode:: ResetPathfindingData() {
        gScore = std::numeric_limits<float>::max();
        hScore = 0.0f;
        cameFrom = nullptr;
    }