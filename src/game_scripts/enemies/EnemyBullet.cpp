#include "./include/game_scripts/enemies/EnemyBullet.h"
#include <TimeSystem.h>
#include <glm/gtc/constants.hpp>
#include <spdlog/spdlog.h>

#include "physics/Body.h"
//#include <Jolt/Physics/Body/BodyFilter.h>
EnemyBullet::EnemyBullet() {
    myNode = GetNode();
}

void EnemyBullet::BulletInTornadoAction(SceneNode* tornadoNode,
                                        float      orbitRadius,
                                        float      rotationSpeed) {
    if (!tornadoNode) return;
    m_Orbiting    = true;
    m_OrbitCenter = tornadoNode;
    m_OrbitRadius = orbitRadius;
    m_OrbitSpeed  = rotationSpeed;

    // Compute starting angle from current position relative to tornado centre
    glm::vec3 toSelf = glm::vec3(myNode->GlobalTransform().Position()) -
                       glm::vec3(tornadoNode->GlobalTransform().Position());
    m_OrbitAngle = glm::degrees(std::atan2(toSelf.x, toSelf.z));

    // Stop straight-line flight
    if (auto* body = myNode->GetObject<Physics::Body>())
        body->SetLinearVelocity(glm::vec3(0.0f));

    spdlog::debug("EnemyBullet: entering tornado orbit (r={:.1f}, speed={:.0f}°/s)",
                  orbitRadius, rotationSpeed);
}

void EnemyBullet::Update() {
    if (!m_Orbiting || !m_OrbitCenter || !myNode) return;

    m_OrbitAngle += m_OrbitSpeed * Time::Delta();

    float rad = glm::radians(m_OrbitAngle);
    glm::vec3 center = m_OrbitCenter->GlobalTransform().Position();
    glm::vec3 orbitPos = center + glm::vec3(
        m_OrbitRadius * std::sin(rad),
        0.0f,
        m_OrbitRadius * std::cos(rad));

    myNode->GlobalTransform().Position() = orbitPos;
}