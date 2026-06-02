#pragma once

#include <SceneComponent.h>
#include <glm/vec3.hpp>
#include <vector>
#include <unordered_map>

class EnemyBase;

struct FlockAgent {
    EnemyBase* ptr      = nullptr;
    glm::vec3  pos      = {};
    glm::vec3  vel      = {};
    glm::vec3  force    = {};   
    int        cellIdx  = -1;   
};

class FlockingSystem : public SceneComponent {
public:
    float separationRadius  = 2.5f;
    float separationWeight  = 1.8f;
    float alignmentRadius   = 5.0f;
    float alignmentWeight   = 0.4f;
    float cohesionRadius    = 6.0f;
    float cohesionWeight    = 0.3f;
    float cellSize          = 6.0f;

    int stuckChecksPerFrame  = 2;
    int astarUpdatesPerFrame = 1;

    explicit FlockingSystem(Scene* scene);
    ~FlockingSystem() override = default;

    void OnPreUpdate()  override;
    void OnPostUpdate() override;
    int  Order()        override { return -10; }

    void Register(EnemyBase* enemy);
    void Unregister(EnemyBase* enemy);

    glm::vec3 GetFlockingForce(EnemyBase* enemy) const;

    bool ShouldUpdateStuck(EnemyBase* enemy) const;
    bool ShouldUpdateAstar(EnemyBase* enemy) const;

private:
    std::vector<FlockAgent> m_Agents;
    std::unordered_map<EnemyBase*, int> m_IndexMap;

    struct Cell {
        int  key   = 0;
        int  start = 0;  
        int  count = 0;
    };
    std::vector<int>  m_CellAgents; 
    std::vector<Cell> m_Cells;
    std::unordered_map<int, int> m_CellLookup; 
    mutable std::vector<int> m_NeighborBuf;

    int m_StuckRR  = 0;
    int m_AstarRR  = 0;
    std::vector<bool> m_StuckThisFrame;
    std::vector<bool> m_AstarThisFrame;

    int  CellKey(const glm::vec3& pos) const;
    void BuildGrid();
    void SyncPositions();
    void ComputeAllForces();
    void ComputeForceFor(int agentIdx);
    void CollectNeighbors(int agentIdx, float radius, std::vector<int>& out) const;
};