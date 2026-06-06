//#include "game_scripts/ThrowableObject.h"
//
//#include <MeshRenderer.h>
//#include <TimeSystem.h>
//#include <spdlog/spdlog.h>
//
//// ---- lifecycle -------------------------------------------------------
//
//void ThrowableObject::Awake() {
//    // Attach an optional visual mesh so the projectile is visible.
//    if (m_VisualMesh && m_VisualMat) {
//        GetNode()->AddObject<MeshRenderer>(m_VisualMesh, m_VisualMat);
//    }
//}
//
//void ThrowableObject::Update() {
//    if (m_HasCollided) return;
//
//    m_ElapsedTime += Time::Delta();
//    if (m_ElapsedTime >= m_MaxLifetime) {
//        spdlog::warn(
//            "ThrowableObject: lifetime ({:.1f}s) exceeded without collision, "
//            "destroying self.",
//            m_MaxLifetime);
//        // Spawn the effect even on timeout so it still applies
//        // if the projectile is sitting on something.
//        SpawnEffect();
//    }
//}
//
//// ---- ICollisionReceiver ----------------------------------------------
//
//void ThrowableObject::OnCollisionEnter(SceneNode* other) {
//    // Ignore self-collisions or already-handled contacts.
//    if (m_HasCollided) return;
//    if (other == GetNode()) return;
//
//    spdlog::debug("ThrowableObject: collision with \"{}\"",
//                  other ? other->GetName() : "<null>");
//
//    m_HasCollided = true;
//    SpawnEffect();
//}
//
//// ---- private ---------------------------------------------------------
//
//void ThrowableObject::SpawnEffect() {
//    if (!m_EffectFactory) {
//        spdlog::warn(
//            "ThrowableObject::SpawnEffect - no EffectFactory set. "
//            "Call SetEffect<T>() before throwing.");
//        GetScene()->QueueDelete(GetNode());
//        return;
//    }
//
//    // Snapshot position before the node is deleted.
//    glm::vec3 spawnPos = GetNode()->GlobalTransform().Position();
//
//    // Create a dedicated node for the effect so its own lifetime
//    // management (QueueDelete inside EffectBase::Update) works
//    // independently of this throwable's node.
//    SceneNode* effectNode = GetScene()->CreateNode("ThrowableEffect");
//    effectNode->GlobalTransform().Position() = spawnPos;
//
//    EffectBase* effect = m_EffectFactory(effectNode);
//
//    if (effect) {
//        spdlog::info(
//            "ThrowableObject: spawned effect at ({:.2f}, {:.2f}, {:.2f})",
//            spawnPos.x, spawnPos.y, spawnPos.z);
//    } else {
//        spdlog::error(
//            "ThrowableObject: EffectFactory returned null; "
//            "check that AddObject<TEffect> succeeded.");
//        // Clean up the empty node we created.
//        GetScene()->QueueDelete(effectNode);
//    }
//
//    // Destroy the projectile node (and this object with it).
//    GetScene()->QueueDelete(GetNode());
//}