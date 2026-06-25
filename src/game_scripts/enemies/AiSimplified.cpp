#include <game_scripts/enemies/AiSimplified.h>
#include <Scene.h>
#include <TimeSystem.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Surface.h"

AiSimplified::AiSimplified()
    : m_Speed(5.0f), m_RotationSpeed(2.0f) {
    myNode   = GetNode();
    m_Body   = nullptr;
    m_Surface = nullptr;
    
  
}

AiSimplified::~AiSimplified() {}

void AiSimplified::EnsureBody() {
    if (!myNode) myNode = GetNode();
    if (!m_Body && myNode)
        m_Body = myNode->GetObject<Physics::Body>();
}

void AiSimplified::ChaseWithSteering(const glm::vec3& flockForce) {

    glm::vec3 toTarget = m_TargetPosition - currentPos;
    toTarget.y = 0.0f;
    float dist = glm::length(toTarget);
    if (dist < 0.1f) { StopMoving(); return; }

    glm::vec3 dir = toTarget / dist;

    glm::vec3 combined = dir + glm::clamp(flockForce, glm::vec3(-0.8f), glm::vec3(0.8f));
    combined.y = 0.0f;
    float len = glm::length(combined);
    if (len > 0.001f) combined /= len;

    MoveInDirection(combined);
}

void AiSimplified::DirectChase() {
    glm::vec3 dir = m_TargetPosition - currentPos;
    dir.y = 0.0f;
    float dist = glm::length(dir);
    if (dist > 0.1f) MoveInDirection(dir / dist);
    else StopMoving();
}

void AiSimplified::Patrol() {
    if (!m_WalkPointSet) {
        SearchWalkPoint();
        return;
    }

    glm::vec3 dir = m_WalkPoint - currentPos;
    float distance = glm::length(dir);

    m_PatrolTimeout += Time::Delta();
    if (m_PatrolTimeout > 5.0f) {
        m_WalkPointSet  = false;
        m_PatrolTimeout = 0.0f;
        return;
    }

    if (distance > 0.5f) MoveInDirection(dir);
    else { StopMoving(); m_WalkPointSet = false; }
}

void AiSimplified::SearchWalkPoint() {
    if (!m_Surface) return;

    if (!m_PatrolPoints.empty()) {
        LookForNextPoint();
    } else {
        float radius = glm::length(m_Surface->GetSize()) * 0.5f;
        m_WalkPoint   = m_Surface->GetRandomWalkPoint(m_Surface->GetCenter(), radius);
        m_WalkPointSet = true;
    }
}

void AiSimplified::LookForNextPoint() {
    m_PosIndex     = (m_PosIndex + 1) % (int)m_PatrolPoints.size();
    m_WalkPoint    = m_PatrolPoints[m_PosIndex];
    m_WalkPointSet = true;
}

void AiSimplified::MoveInDirection(const glm::vec3& direction) {
    if (!myNode || !m_Body) return;
    if (glm::length(direction) < 0.001f) { StopMoving(); return; }

    glm::vec3 dir    = glm::normalize(direction);
    glm::vec3 newVel = dir * m_Speed;
    newVel.y = m_Body->GetLinearVelocity().y;

    if (m_Surface) {
        glm::vec3 predicted = currentPos + newVel * Time::Delta();
        if (!m_Surface->ContainsPoint(predicted, 0.2f)) {
            StopMoving();
            return;
        }
    }

    m_Body->SetLinearVelocity(newVel);
    RotateNode(dir);
}

void AiSimplified::StopMoving() {
    glm::vec3 v = m_Body->GetLinearVelocity();
    m_Body->SetLinearVelocity(glm::vec3(0, v.y, 0));
}

void AiSimplified::RotateNode(glm::vec3 dir) {
    if (!myNode || !m_Body) return;
    if (glm::length(dir) < 0.01f) return;
    dir = glm::normalize(dir);
    float     targetYaw = atan2(dir.x, dir.z);
    glm::quat targetRot = glm::angleAxis(targetYaw, glm::vec3(0, 1, 0));
    glm::quat currentRot = myNode->GlobalTransform().Rotation();
    glm::quat newRot = glm::slerp(currentRot, targetRot, m_RotationSpeed * Time::Delta());
    m_Body->SetRotation(newRot);
    myNode->GlobalTransform().Rotation() = newRot;
    m_Body->SetAngularVelocity(glm::vec3(0, 0, 0));
}

void AiSimplified::LockXZRotation() {
    if (!m_Body) return;
    glm::quat rot = m_Body->GetRotation();
    const float epsilon = 0.001f;
    if (glm::abs(rot.x) > epsilon || glm::abs(rot.z) > epsilon) {
        glm::quat corrected = glm::normalize(glm::quat(rot.w, 0.0f, rot.y, 0.0f));
        m_Body->SetRotation(corrected);
        myNode->GlobalTransform().Rotation() = corrected;
        glm::vec3 angVel = m_Body->GetAngularVelocity();
        m_Body->SetAngularVelocity(glm::vec3(0.0f, angVel.y, 0.0f));
    }
}

void AiSimplified::Flee() {
    glm::vec3 away = currentPos - m_TargetPosition;
    away.y = 0.0f;
    float len = glm::length(away);
    if (len < 0.1f) { StopMoving(); return; }
    MoveInDirection(away / len);
}

void AiSimplified::SetTarget(glm::vec3 target) { m_TargetPosition = target; }

void AiSimplified::SetSurface(Surface* surface) {
    if (surface) { m_Surface = surface; }
}

void AiSimplified::SetPatrolPoints(const std::vector<glm::vec2>& points) {
    m_PatrolPoints.clear();
    for (const auto& p : points) {
        float y = m_Surface ? m_Surface->GetGroundHeight(p.x, p.y) : 0.0f;
        m_PatrolPoints.push_back({p.x, y, p.y});
    }
}