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
    m_PatrolTargets.push_back({});
    m_PatrolDirty.push_back(true); 
    spdlog::info("FlockingSystem: registered (total={})", m_Agents.size());
}

void FlockingSystem::Unregister(EnemyBase* enemy) {
    auto it = m_IndexMap.find(enemy);
    if (it == m_IndexMap.end()) return;

    int idx  = it->second;
    int last = (int)m_Agents.size() - 1;

    if (idx != last) {
        m_Agents[idx]        = m_Agents[last];
        m_PatrolTargets[idx] = m_PatrolTargets[last];
        m_PatrolDirty[idx]   = m_PatrolDirty[last];
        m_IndexMap[m_Agents[idx].ptr] = idx;
    }
    m_Agents.pop_back();
    m_PatrolTargets.pop_back();
    m_PatrolDirty.pop_back();
    m_IndexMap.erase(it);
}

glm::vec3 FlockingSystem::GetFlockingForce(EnemyBase* enemy) const {
    auto it = m_IndexMap.find(enemy);
    return (it != m_IndexMap.end()) ? m_Agents[it->second].force : glm::vec3(0.0f);
}

glm::vec3 FlockingSystem::GetPatrolTarget(EnemyBase* enemy) const {
    auto it = m_IndexMap.find(enemy);
    return (it != m_IndexMap.end()) ? m_PatrolTargets[it->second] : glm::vec3(0.0f);
}

void FlockingSystem::RefreshPatrolTarget(EnemyBase* enemy) {
    auto it = m_IndexMap.find(enemy);
    if (it != m_IndexMap.end())
        m_PatrolDirty[it->second] = true;
}

void FlockingSystem::OnPreUpdate() {
    if (m_Agents.empty()) return;

    SyncPositions();   // pobierz pos/vel z physics body
    BuildGrid();       // spatial hash grid
    ComputeAllForces(); // separation + alignment + cohesion
    UpdatePatrolTargets(); // max 2 punkty patrol/klatkę
}

void FlockingSystem::SyncPositions() {
    for (auto& a : m_Agents) {
        if (!a.ptr || !a.ptr->m_Body) continue;
        a.pos = a.ptr->currentPos;
        glm::vec3 v = a.ptr->m_Body->GetLinearVelocity();
        v.y  = 0.0f;
        a.vel = v;
    }
}

int FlockingSystem::CellKey(const glm::vec3& pos) const {
    int cx = (int)std::floor(pos.x / cellSize) + 1000;
    int cz = (int)std::floor(pos.z / cellSize) + 1000;
    return cx * 2003 + cz;
}

void FlockingSystem::BuildGrid() {
    const int n = (int)m_Agents.size();
    m_CellLookup.clear();
    m_Cells.clear();
    m_CellAgents.resize(n);

    for (int i = 0; i < n; ++i)
        m_Agents[i].cellIdx = CellKey(m_Agents[i].pos);

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
        offset    += cell.count;
        cell.count = 0;
    }

    for (int i = 0; i < n; ++i) {
        auto& cell = m_Cells[m_CellLookup[m_Agents[i].cellIdx]];
        m_CellAgents[cell.start + cell.count++] = i;
    }
}

void FlockingSystem::CollectNeighbors(int agentIdx, float radius,
                                       std::vector<int>& out) const {
    out.clear();
    const glm::vec3& pos = m_Agents[agentIdx].pos;
    float r2     = radius * radius;
    int   cellR  = (int)std::ceil(radius / cellSize) + 1;
    int   cx     = (int)std::floor(pos.x / cellSize) + 1000;
    int   cz     = (int)std::floor(pos.z / cellSize) + 1000;

    for (int dx = -cellR; dx <= cellR; ++dx) {
        for (int dz = -cellR; dz <= cellR; ++dz) {
            auto it = m_CellLookup.find((cx + dx) * 2003 + (cz + dz));
            if (it == m_CellLookup.end()) continue;
            const Cell& cell = m_Cells[it->second];
            for (int i = cell.start; i < cell.start + cell.count; ++i) {
                int nb = m_CellAgents[i];
                if (nb == agentIdx) continue;
                glm::vec3 d = m_Agents[nb].pos - pos;
                d.y = 0.0f;
                if (glm::dot(d, d) <= r2) out.push_back(nb);
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

    glm::vec3 sep(0.0f), ali(0.0f), coh(0.0f);
    int sc = 0, ac = 0, cc = 0;

    float sep2 = separationRadius * separationRadius;
    float ali2 = alignmentRadius  * alignmentRadius;
    float coh2 = cohesionRadius   * cohesionRadius;

    for (int nb : m_NeighborBuf) {
        glm::vec3 d = m_Agents[nb].pos - myPos;
        d.y = 0.0f;
        float d2 = glm::dot(d, d);

        if (d2 < sep2 && d2 > 0.0001f) {
            float dist = std::sqrt(d2);
            sep -= (d / dist) / dist;
            ++sc;
        }
        if (d2 < ali2) { ali += m_Agents[nb].vel; ++ac; }
        if (d2 < coh2) { coh += m_Agents[nb].pos; ++cc; }
    }

    glm::vec3 force(0.0f);

    if (sc > 0) {
        sep /= (float)sc;
        float l = glm::length(sep);
        if (l > 0.001f) force += (sep / l) * separationWeight;
    }
    if (ac > 0) {
        ali /= (float)ac;
        glm::vec3 steer = ali - myVel;
        float l = glm::length(steer);
        if (l > 0.001f) force += (steer / l) * alignmentWeight;
    }
    if (cc > 0) {
        coh /= (float)cc;
        glm::vec3 toC = coh - myPos;
        toC.y = 0.0f;
        float l = glm::length(toC);
        if (l > 0.001f) force += (toC / l) * cohesionWeight;
    }

    m_Agents[idx].force = force;
}


void FlockingSystem::UpdatePatrolTargets() {
    const int n = (int)m_Agents.size();
    if (n == 0) return;

    int refreshed = 0;
    const int maxPerFrame = 2;

    for (int i = 0; i < n && refreshed < maxPerFrame; ++i) {
        int idx = (m_PatrolRR + i) % n;
        if (!m_PatrolDirty[idx]) continue;

        EnemyBase* e = m_Agents[idx].ptr;
        if (!e || !e->GetSurface()) continue;

        Surface* surf   = e->GetSurface();
        float    radius = glm::length(surf->GetSize()) * 0.5f;
        m_PatrolTargets[idx] = surf->GetRandomWalkPoint(surf->GetCenter(), radius);
        m_PatrolDirty[idx]   = false;
        ++refreshed;
    }

    m_PatrolRR = (m_PatrolRR + maxPerFrame) % n;
}