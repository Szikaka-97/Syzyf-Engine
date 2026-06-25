#include "./include/game_scripts/enemies/BeetrootSegment.h"
#include "./include/game_scripts/enemies/EnemyBeetroot.h"
#include "game_scripts/PlayerController.h"
#include <glm/glm.hpp>
#include "./include/game_scripts/enemies/EnemyBase.h"

void BeetrootSegment::Initialize(EnemyBeetroot* owner, SceneNode* playerNode) {
    m_Owner      = owner;
    m_PlayerNode = playerNode;
    myNode = GetNode();

    SceneNode* enemyModel =
         ResourceDatabase::Global->Get<GltfScene>("./res/models/enemies/burak_segment.glb")
             ->Instantiate(GetScene(), GetNode(), "BeetrootSegmentModel");
    enemyModel->LocalTransform().Position() = glm::zero<glm::vec3>();

    AnimationComponent* enemyAnim = myNode->GetObjectInChildren<AnimationComponent>();
    if (enemyAnim) {
        enemyAnim->Play("ArmatureAction");
    }
}

void BeetrootSegment::Update() {
    if (m_HasHit || !m_PlayerNode || !myNode) return;

    glm::vec3 segPos    = myNode->GlobalTransform().Position();
    glm::vec3 playerPos = m_PlayerNode->GlobalTransform().Position();

    if (glm::distance(segPos, playerPos) <= m_HitRadius) {
        m_HasHit = true;

        auto* pc = m_PlayerNode->GetObject<PlayerController>();
        if (pc) pc->TakeDamage(15);

        if (m_Owner) m_Owner->OnSegmentHitPlayer();
    }
}