#include "game_scripts/enemies/FlockingSystem.h"
#include "game_scripts/enemies/EnemyBase.h"
#include <Scene.h>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>

FlockingSystem::FlockingSystem(Scene* scene) : SceneComponent(scene) {
    m_NeighborBuf.reserve(32);
    spdlog::info("FlockingSystem: initialized");
}

void FlockingSystem::Register(EnemyBase* enemy) {
    if (!enemy || m_IndexMap.count(enemy)) return;
    int idx = (int)m_Agents.size();
    m_Agents.push_back({enemy, {}, {}, {}, -1});
    m_IndexMap[enemy] = idx;
    m_StuckThisFrame.push_back(false);
    m_AstarThisFrame.push_back(false);
    spdlog::debug("FlockingSystem: registered (total={})", m_Agents.size());
}

void FlockingSystem::Unregister(EnemyBase* enemy) {
    auto it = m_IndexMap.find(enemy);
    if (it == m_IndexMap.end()) return;

    int idx  = it->second;
    int last = (int)m_Agents.size() - 1;

    if (idx != last) {
        m_Agents[idx] = m_Agents[last];
        m_IndexMap[m_Agents[idx].ptr] = idx;
        m_StuckThisFrame[idx] = m_StuckThisFrame[last];
        m_AstarThisFrame[idx] = m_AstarThisFrame[last];
    }
    m_Agents.pop_back();
    m_StuckThisFrame.pop_back();
    m_AstarThisFrame.pop_back();
    m_IndexMap.erase(it);
}

glm::vec3 FlockingSystem::GetFlockingForce(EnemyBase* enemy) const {
    auto it = m_IndexMap.find(enemy);
    if (it == m_IndexMap.end()) return glm::vec3(0.0f);
    return m_Agents[it->second].force;
}

bool FlockingSystem::ShouldUpdateStuck(EnemyBase* enemy) const {
    auto it = m_IndexMap.find(enemy);
    if (it == m_IndexMap.end()) return false;
    return m_StuckThisFrame[it->second];
}

bool FlockingSystem::ShouldUpdateAstar(EnemyBase* enemy) const {
    auto it = m_IndexMap.find(enemy);
    if (it == m_IndexMap.end()) return false;
    return m_AstarThisFrame[it->second];
}

void FlockingSystem::OnPreUpdate() {
    const int n = (int)m_Agents.size();
    if (n == 0) return;

    SyncPositions();

    BuildGrid();

    ComputeAllForces();

    std::fill(m_StuckThisFrame.begin(), m_StuckThisFrame.end(), false);
    std::fill(m_AstarThisFrame.begin(), m_AstarThisFrame.end(), false);

    for (int i = 0; i < stuckChecksPerFrame && i < n; ++i) {
        m_StuckThisFrame[m_StuckRR % n] = true;
        m_StuckRR = (m_StuckRR + 1) % n;
    }
    for (int i = 0; i < astarUpdatesPerFrame && i < n; ++i) {
        m_AstarThisFrame[m_AstarRR % n] = true;
        m_AstarRR = (m_AstarRR + 1) % n;
    }
}

void FlockingSystem::OnPostUpdate() {
}


void FlockingSystem::SyncPositions() {
    for (auto& agent : m_Agents) {
        if (agent.ptr && agent.ptr->m_Body) {
            agent.pos = agent.ptr->currentPos;    
            glm::vec3 v = agent.ptr->m_Body->GetLinearVelocity();
            v.y = 0.0f;
            agent.vel = v;
        }
    }
}


int FlockingSystem::CellKey(const glm::vec3& pos) const {
    int cx = (int)std::floor(pos.x / cellSize);
    int cz = (int)std::floor(pos.z / cellSize);
    cx += 1000; cz += 1000;
    return cx * 2003 + cz;
}

void FlockingSystem::BuildGrid() {
    const int n = (int)m_Agents.size();
    m_CellLookup.clear();
    m_Cells.clear();
    m_CellAgents.resize(n);

    for (int i = 0; i < n; ++i) {
        m_Agents[i].cellIdx = CellKey(m_Agents[i].pos);
    }

    for (int i = 0; i < n; ++i) {
        int key = m_Agents[i].cellIdx;
        if (!m_CellLookup.count(key)) {
            m_CellLookup[key] = (int)m_Cells.size();
            m_Cells.push_back({key, 0, 0});
        }
        m_Cells[m_CellLookup[key]].count++;
    }

    int offset = 0;
    for (auto& cell : m_Cells) {
        cell.start = offset;
        offset += cell.count;
        cell.count = 0; 
    }

    for (int i = 0; i < n; ++i) {
        int cellIdx = m_CellLookup[m_Agents[i].cellIdx];
        auto& cell  = m_Cells[cellIdx];
        m_CellAgents[cell.start + cell.count] = i;
        cell.count++;
    }
}

void FlockingSystem::CollectNeighbors(int agentIdx, float radius,
                                       std::vector<int>& out) const {
    out.clear();
    const glm::vec3& pos = m_Agents[agentIdx].pos;
    float r2 = radius * radius;
    int cellR = (int)std::ceil(radius / cellSize) + 1;

    int cx = (int)std::floor(pos.x / cellSize) + 1000;
    int cz = (int)std::floor(pos.z / cellSize) + 1000;

    for (int dx = -cellR; dx <= cellR; ++dx) {
        for (int dz = -cellR; dz <= cellR; ++dz) {
            int key = (cx + dx) * 2003 + (cz + dz);
            auto it = m_CellLookup.find(key);
            if (it == m_CellLookup.end()) continue;
            const Cell& cell = m_Cells[it->second];
            for (int i = cell.start; i < cell.start + cell.count; ++i) {
                int nbIdx = m_CellAgents[i];
                if (nbIdx == agentIdx) continue;
                glm::vec3 diff = m_Agents[nbIdx].pos - pos;
                diff.y = 0.0f;
                if (glm::dot(diff, diff) <= r2)
                    out.push_back(nbIdx);
            }
        }
    }
}

void FlockingSystem::ComputeAllForces() {
    for (int i = 0; i < (int)m_Agents.size(); ++i)
        ComputeForceFor(i);
}

void FlockingSystem::ComputeForceFor(int idx) {
    float maxR = std::max({separationRadius, alignmentRadius, cohesionRadius});
    CollectNeighbors(idx, maxR, m_NeighborBuf);

    const glm::vec3& myPos = m_Agents[idx].pos;
    const glm::vec3& myVel = m_Agents[idx].vel;

    glm::vec3 separation(0.0f), alignment(0.0f), cohesion(0.0f);
    int sepCount = 0, aliCount = 0, cohCount = 0;

    float sep2 = separationRadius * separationRadius;
    float ali2 = alignmentRadius  * alignmentRadius;
    float coh2 = cohesionRadius   * cohesionRadius;

    for (int nbIdx : m_NeighborBuf) {
        glm::vec3 diff = m_Agents[nbIdx].pos - myPos;
        diff.y = 0.0f;
        float d2 = glm::dot(diff, diff);

        // Separation
        if (d2 < sep2 && d2 > 0.0001f) {
            float dist = std::sqrt(d2);
            separation -= (diff / dist) / dist; 
            sepCount++;
        }
        // Alignment
        if (d2 < ali2) {
            alignment += m_Agents[nbIdx].vel;
            aliCount++;
        }
        // Cohesion
        if (d2 < coh2) {
            cohesion += m_Agents[nbIdx].pos;
            cohCount++;
        }
    }

    glm::vec3 force(0.0f);

    if (sepCount > 0) {
        separation /= (float)sepCount;
        float len = glm::length(separation);
        if (len > 0.001f) force += (separation / len) * separationWeight;
    }

    if (aliCount > 0) {
        alignment /= (float)aliCount;
        glm::vec3 steer = alignment - myVel;
        float len = glm::length(steer);
        if (len > 0.001f) force += (steer / len) * alignmentWeight;
    }

    if (cohCount > 0) {
        cohesion /= (float)cohCount;
        glm::vec3 toCenter = cohesion - myPos;
        toCenter.y = 0.0f;
        float len = glm::length(toCenter);
        if (len > 0.001f) force += (toCenter / len) * cohesionWeight;
    }

    m_Agents[idx].force = force;
}