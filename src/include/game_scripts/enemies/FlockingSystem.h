#pragma once

#include <SceneComponent.h>
#include <glm/vec3.hpp>
#include <vector>
#include <unordered_map>

class EnemyBase;

struct FlockAgent {
    EnemyBase* ptr     = nullptr;
    glm::vec3  pos     = {};
    glm::vec3  vel     = {};
    glm::vec3  force   = {};  // wynik separation+alignment+cohesion
    int        cellIdx = -1;
};

// Centralny system AI — oblicza flocking i patrol dla wszystkich enemy raz/klatkę.
// Zero raycastów. Zero FindObjectsOfType. Zero A*.
// Separation zastępuje unikanie kolizji między agentami.
class FlockingSystem : public SceneComponent {
public:
    float separationRadius = 2.5f;
    float separationWeight = 2.0f;  // wyższe — główny mechanizm unikania kolizji
    float alignmentRadius  = 5.0f;
    float alignmentWeight  = 0.3f;
    float cohesionRadius   = 6.0f;
    float cohesionWeight   = 0.2f;
    float cellSize         = 6.0f;

    explicit FlockingSystem(Scene* scene);
    ~FlockingSystem() override = default;

    void OnPreUpdate() override;
    void OnPostUpdate() override {}
    int  Order()       override { return -10; }

    void Register(EnemyBase* enemy);
    void Unregister(EnemyBase* enemy);

    // O(1) — odczyt z tablicy
    glm::vec3 GetFlockingForce(EnemyBase* enemy) const;

    // Zwraca punkt patrol dla danego agenta (z cache, bez losowania co klatkę)
    glm::vec3 GetPatrolTarget(EnemyBase* enemy) const;
    void      RefreshPatrolTarget(EnemyBase* enemy);  // wołaj gdy enemy dotarł do celu

private:
    std::vector<FlockAgent>             m_Agents;
    std::unordered_map<EnemyBase*, int> m_IndexMap;

    // Patrol targets: każdy enemy ma swój punkt, odświeżany leniwie
    std::vector<glm::vec3> m_PatrolTargets;
    std::vector<bool>      m_PatrolDirty;  // true = potrzebuje nowego punktu
    int                    m_PatrolRR = 0; // round-robin odświeżania

    // Spatial grid (flat arrays)
    struct Cell { int key = 0, start = 0, count = 0; };
    std::vector<int>             m_CellAgents;
    std::vector<Cell>            m_Cells;
    std::unordered_map<int, int> m_CellLookup;
    mutable std::vector<int>     m_NeighborBuf;

    int  CellKey(const glm::vec3& pos) const;
    void SyncPositions();
    void BuildGrid();
    void ComputeAllForces();
    void ComputeForceFor(int idx);
    void CollectNeighbors(int idx, float radius, std::vector<int>& out) const;
    void UpdatePatrolTargets();  // max 2 odświeżenia/klatkę
};