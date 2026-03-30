#pragma once

#include <GameObject.h>
#include <vector>
#include <glm/glm.hpp>

class Mesh;

class Surface : public GameObject {
private:
    Mesh* floorMesh;
    std::vector<glm::vec3> walkablePoints;
    float cellSize;
    void CollectVertices();

    // convert mesh to grid of vertices
    void GenerateGrid(float minX, float maxX, float minZ, float maxZ);

public:
    Surface(Mesh* floorMesh, float cellSize = 1.0f);
    ~Surface();

    glm::vec3 GetRandomWalkPoint(const glm::vec3& center, float radius) const;

    bool IsOnSurface(const glm::vec3& point) const;

    float GetGroundHeight(float x, float z) const;
};