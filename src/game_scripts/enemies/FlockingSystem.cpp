#include "game_scripts/enemies/FlockingSystem.h"

#include "TimeSystem.h"
#include "game_scripts/enemies/EnemyBase.h"
#include <Scene.h>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>

FlockingSystem::FlockingSystem(Scene* scene) : SceneComponent(scene) {
    //m_NeighborBuf.reserve(32);
    spdlog::info("FlockingSystem: initialized");
}

void FlockingSystem::Register(EnemyBase* enemy) {
    if (!enemy || m_IndexMap.count(enemy)) return;
    int idx = (int)m_Agents.size();
    m_Agents.push_back({enemy, {}, {}, {}, -1});
    m_IndexMap[enemy] = idx;
    m_PatrolTargets.push_back({});
    m_PatrolDirty.push_back(true);
}

void FlockingSystem::Unregister(EnemyBase* enemy) {
    auto it = m_IndexMap.find(enemy);
    if (it == m_IndexMap.end()) return;
    int idx = it->second;
    int last = (int)m_Agents.size() - 1;
    if (idx != last) {
        m_Agents[idx] = m_Agents[last];
        m_PatrolTargets[idx] = m_PatrolTargets[last];
        m_PatrolDirty[idx] = m_PatrolDirty[last];
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
    if (it != m_IndexMap.end()) m_PatrolDirty[it->second] = true;
}

void FlockingSystem::OnPreUpdate() {
    if (m_Agents.empty()) return;

    UpdatePatrolTargets();
    float dt = Time::Delta();
    float sepRadiusSq = separationRadius * separationRadius;

    for (auto& a : m_Agents) {
        if (a.ptr && a.ptr->m_Body) {
            a.pos = a.ptr->currentPos;
        }
    }

    for (int i = 0; i < (int)m_Agents.size(); ++i) {
        EnemyBase* enemy = m_Agents[i].ptr;

        if (!enemy) continue;

        enemy->EnsureBody();

        if (!enemy->m_Body || enemy->m_hp <= 0) continue;

        if (enemy->m_InAttackAnimation || enemy->currentState == States::ATTACKING) {
            enemy->StopMoving();
            continue;
        }

        enemy->LockXZRotation();

        glm::vec3 myPos = m_Agents[i].pos;

        if (enemy->currentState == States::CHASING) {
            glm::vec3 toTarget = enemy->m_TargetPosition - myPos;
            toTarget.y = 0.0f;
            if (glm::length(toTarget) <= enemy->attackRange * 0.85f) {
                enemy->StopMoving();
                continue;
            }
        }

        glm::vec3 separation(0.0f);
        for (int j = 0; j < (int)m_Agents.size(); ++j) {
            if (i == j) continue;
            glm::vec3 diff = myPos - m_Agents[j].pos;
            diff.y = 0.0f;
            float distSq = glm::dot(diff, diff);
            if (distSq < sepRadiusSq) {
                if (distSq > 0.0001f)
                    separation += diff * (1.0f / distSq);
                else
                    separation += glm::vec3(0.5f * ((i % 3) - 1), 0.0f, 0.5f * ((j % 3) - 1));
            }
        }

        glm::vec3 targetDir(0.0f);
        if (enemy->currentState == States::CHASING || enemy->currentState == States::FLEEING) {
            targetDir = enemy->m_TargetPosition - myPos;
            if (enemy->currentState == States::FLEEING) targetDir = -targetDir;
        } else if (enemy->currentState == States::PATROLLING) {
            targetDir = m_PatrolTargets[i] - myPos;
            if (glm::dot(targetDir, targetDir) < 0.36f) {
                RefreshPatrolTarget(enemy);
                enemy->StopMoving();
                continue;
            }
        }

        targetDir.y = 0.0f;
        float targetLenSq = glm::dot(targetDir, targetDir);

        if (targetLenSq > 0.001f) {
            targetDir /= std::sqrt(targetLenSq);
            glm::vec3 finalDir = targetDir + (separation * separationWeight);
            finalDir.y = 0.0f;
            float finalLenSq = glm::dot(finalDir, finalDir);
            if (finalLenSq > 0.001f)
                finalDir /= std::sqrt(finalLenSq);

            glm::vec3 currentVel = enemy->m_Body->GetLinearVelocity();
            glm::vec3 targetVel  = finalDir * enemy->GetMovementSpeed();
            glm::vec3 newVel;
            newVel.x = glm::mix(currentVel.x, targetVel.x, dt * 8.0f);
            newVel.z = glm::mix(currentVel.z, targetVel.z, dt * 8.0f);
            newVel.y = currentVel.y;

            enemy->m_Body->SetLinearVelocity(newVel);
            enemy->RotateNode(finalDir);
        } else {
            enemy->StopMoving();
        }
    }
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
