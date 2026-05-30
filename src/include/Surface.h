#pragma once

#include <GameObject.h>
#include <vector>
#include <glm/glm.hpp>
#include "physics/DebugRenderer.h"
#include "./include/game_scripts/enemies/AiSimplified.h"

class Mesh;
class AiNode;
class AiSimplified;

class Surface : public GameObject {
private:
    Mesh* floorMesh;

    float cellSize;
    std::vector<glm::vec3> walkablePoints;
    glm::vec3 m_center;  
    glm::vec3 m_size;
    int m_RoomID;
    std::vector<AiSimplified*> myEnemies;
    bool m_playerInside;

    void CollectVertices();
    void CalculateBounds();   

    // convert mesh to grid of vertices
    void GenerateGrid(float minX, float maxX, float minZ, float maxZ);
    bool IsPointBlocked(const glm::vec3& point) const;

public:

    Surface(Mesh* floorMesh, float cellSize = 1.0f);
    ~Surface();

    glm::vec3 GetRandomWalkPoint(const glm::vec3& center, float radius) const;

    bool IsOnSurface(const glm::vec3& point) const;

    float GetGroundHeight(float x, float z) const;
    void SetGroundHeight( float height);
    const std::vector<glm::vec3>& GetWalkablePoints() const { return walkablePoints; }
    glm::vec3 GetCenter() const { return m_center; }
    glm::vec3 GetSize()   const { return m_size; }
    int GetID() const { return m_RoomID; }
    std::vector<AiSimplified*> GetEnemies() const {return myEnemies;};
    void InformEnter() ; 
    void InformExit() ;
    void SetID(int id) { m_RoomID = id; }
    void SetEnemies(const std::vector<AiSimplified*>& enemies) { myEnemies = enemies; }
    void AddEnemy(AiSimplified* enemy) { myEnemies.push_back(enemy); }
    bool ContainsPoint (const glm::vec3& point, float margin) ;
    void DrawDebugSurface(Physics::DebugRenderer* debugRenderer, float pointSize = 0.1f, int step = 5) const;

    void Update();
};