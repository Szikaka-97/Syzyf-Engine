#pragma once

#include <GameObject.h>
#include <vector>
#include <glm/glm.hpp>
#include "physics/DebugRenderer.h"

class Mesh;

class Surface : public GameObject {
private:
    Mesh* floorMesh;
    std::vector<glm::vec3> walkablePoints;
    float cellSize;
    glm::vec3 m_center;  
    glm::vec3 m_size;     

    void CollectVertices();
    void CalculateBounds();   

    // convert mesh to grid of vertices
    void GenerateGrid(float minX, float maxX, float minZ, float maxZ);

public:
    Surface(Mesh* floorMesh, float cellSize = 1.0f);
    ~Surface();

    glm::vec3 GetRandomWalkPoint(const glm::vec3& center, float radius) const;

    bool IsOnSurface(const glm::vec3& point) const;

    float GetGroundHeight(float x, float z) const;
    const std::vector<glm::vec3>& GetWalkablePoints() const { return walkablePoints; }
    glm::vec3 GetCenter() const { return m_center; }
    glm::vec3 GetSize()   const { return m_size; }

    void DrawDebugSurface(Physics::DebugRenderer* debugRenderer, float pointSize = 0.1f, int step = 5) const;

};