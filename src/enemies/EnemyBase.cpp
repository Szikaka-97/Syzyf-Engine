#include <AiSimplified.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <MeshRenderer.h>
#include <TimeSystem.h>
#include <physics/LayerMaskFilter.h>

#include "EnemyBase.h"

EnemyBase::EnemyBase()
    : fov(glm::radians(180.0f)),
      m_AttackCooldown(1.5f),
      m_AttackTimer(0.0f),
      m_ProjectileSpeed(15.0f),
      m_ProjectileMesh(nullptr),
      m_ProjectileMaterial(nullptr),
      m_hp(100) {
  // myNode = GetNode();
  // m_Body = myNode->GetObject<Physics::Body>();
}

EnemyBase::~EnemyBase() {}

void EnemyBase::Attack() {
  // if (!m_TargetPosition) return;
  if (m_InAttackAnimation) return;

  glm::vec3 dirTo = m_TargetPosition - currentPos;
  if (glm::length(dirTo) > 0.01f) RotateNode(dirTo);

  m_AttackTimer += Time::Delta();
  if (m_AttackTimer >= m_AttackCooldown) {
    m_AttackTimer = 0.0f;
    SetAnimation("attack.001");
    m_InAttackAnimation = true;
    m_AttackAnimationElapsed = 0.0f;

    PlayAttackAnimation("attack.001");
    SpawnProjectile(m_TargetPosition);
  }
}

void EnemyBase::Die() {
  if (myNode) {
    GetScene()->QueueDelete(myNode);
    myNode = nullptr;
  }
}
void EnemyBase::SpawnProjectile(const glm::vec3& targetPos) {
  if (!m_ProjectileMesh || !m_ProjectileMaterial) {
    spdlog::warn("AiNode: Projectile resources not set!");
    return;
  }

  glm::vec3 startPos = currentPos + glm::vec3(0.0f, 1.0f, 0.0f);
  glm::vec3 dir = glm::normalize(targetPos - startPos);

  auto* projectileNode = GetScene()->CreateNode("EnemyProjectile");
  projectileNode->AddObject<MeshRenderer>(m_ProjectileMesh,
                                          m_ProjectileMaterial);
  projectileNode->GlobalTransform().Position() = startPos;
  projectileNode->GlobalTransform().Scale() = glm::vec3(0.2f);

  JPH::BodyCreationSettings projectileSettings(
      new JPH::SphereShape(0.2f),
      JPH::RVec3(startPos.x, startPos.y, startPos.z), JPH::Quat::sIdentity(),
      JPH::EMotionType::Dynamic, Physics::Layers::MOVING);

  auto* body = projectileNode->AddObject<Physics::Body>(projectileSettings);

  JPH::Vec3 jphVel = JPH::Vec3(dir.x, dir.y, dir.z) * m_ProjectileSpeed;
  body->SetLinearVelocity(
      glm::vec3(jphVel.GetX(), jphVel.GetY(), jphVel.GetZ()));

  body->SetRestitution(0.3f);
  body->SetFriction(0.5f);
  // body->Awake();
}

void EnemyBase::UpdateAttackAnimation() {
  if (!m_InAttackAnimation) return;

  m_AttackAnimationElapsed += Time::Delta();
  if (m_AttackAnimationElapsed >= m_AttackAnimationDuration) {
    m_InAttackAnimation = false;

    if (glm::length(m_Body->GetLinearVelocity()) > 0.1f)
      SetAnimation("idle.001");
    else
      SetAnimation("stop.001");
  }
}

void EnemyBase::SetAnimation(const std::string& name) {
  if (!m_AttackAnimation) return;
  if (m_CurrentAnimation == name) return;
  m_AttackAnimation->Play(name);
  m_CurrentAnimation = name;
  spdlog::debug("AiNode: changed animation to {}", name);
}

bool EnemyBase::CanSeePlayer() const {
  if (!m_Body) return false;
  auto* physics = GetScene()->GetComponent<Physics::System>();
  if (!physics) return false;

  glm::vec3 origin = currentPos + glm::vec3(0.0f, 1.0f, 0.0f);
  glm::vec3 dir = m_TargetPosition - origin;
  float dist = glm::length(dir);
  if (dist < 0.1f) return true;

  Physics::LayerMaskFilter filter({}, false);
  filter.IgnoreBody(m_Body->GetBodyID());

  SceneNode* hit = physics->CastRay(origin, dir * dist, {}, {}, filter).node;
  if (!hit) return true;
  return (glm::vec3(hit->GlobalTransform().Position()) == m_TargetPosition);
}

void EnemyBase::SetProjectileResources(Mesh* mesh, Material* material) {
  m_ProjectileMesh = mesh;
  m_ProjectileMaterial = material;
}

void EnemyBase::SetAttackCooldown(float cooldown) {
  m_AttackCooldown = cooldown;
}

void EnemyBase::SetAttackAnimation(AnimationComponent* anim) {
  m_AttackAnimation = anim;
  if (anim) {
    for (const auto& a : anim->animations) {
      if (a.data.name == "attack.001") {
        m_AttackAnimationDuration = a.data.duration;
        spdlog::info("Attack anim duration = {:.2f}s",
                     m_AttackAnimationDuration);
        break;
      }
    }
  }
}

void EnemyBase::PlayAttackAnimation(std::string name) {
  if (m_AttackAnimation) {
    m_AttackAnimation->Play(name);
  }
}

void EnemyBase::OnPlayerEnteredRoom() { isPlayerInRoom = true; }

void EnemyBase::OnPlayerExitedRoom() {
  m_UsingAStar = false;
  m_Path.clear();
  m_StuckTimer = 0.0f;
  isPlayerInRoom = false;
}

void EnemyBase::DrawDebugView() {
  if (!myNode) return;

  /*auto* scene = GetScene();
  Physics::DebugRenderer* debugRenderer = scene ?
  scene->GetComponent<Physics::DebugRenderer>() : nullptr; if (!debugRenderer) {
          return;
  }*/
  if (!myNode) return;
  auto* debugRenderer =
      static_cast<Physics::DebugRenderer*>(JPH::DebugRenderer::sInstance);
  if (!debugRenderer) return;

  if (m_Surface) {
    m_Surface->DrawDebugSurface(debugRenderer, 0.5f, 1);
  }

  int segments = 24;

  glm::quat rotation = myNode->GlobalTransform().Rotation();
  glm::vec3 forward = rotation * glm::vec3(0, 0, 1);
  forward = glm::normalize(glm::vec3(forward.x, 0, forward.z));

  // glm::vec3 pos = transform;

  std::vector<glm::vec3> arcPoints;
  float startAngle = atan2(forward.x, forward.z) - fov / 2.0f;
  for (int i = 0; i <= segments; ++i) {
    float t = (float)i / segments;
    float angle = startAngle + t * fov;
    float x = sightRange * sin(angle);
    float z = sightRange * cos(angle);
    arcPoints.push_back(currentPos + glm::vec3(x, 0, z));
  }

  for (const auto& p : arcPoints) {
    debugRenderer->DrawLine(JPH::Vec3(currentPos.x, currentPos.y, currentPos.z),
                            JPH::Vec3(p.x, p.y, p.z), JPH::Color::sPurple);
  }

  for (size_t i = 0; i < arcPoints.size() - 1; ++i) {
    debugRenderer->DrawLine(
        JPH::Vec3(arcPoints[i].x, arcPoints[i].y, arcPoints[i].z),
        JPH::Vec3(arcPoints[i + 1].x, arcPoints[i + 1].y, arcPoints[i + 1].z),
        JPH::Color::sPurple);
  }
  if (m_Path.size() >= 2) {
    for (size_t i = 0; i < m_Path.size() - 1; ++i) {
      debugRenderer->DrawLine(
          JPH::Vec3(m_Path[i].x, m_Path[i].y + 0.2f, m_Path[i].z),
          JPH::Vec3(m_Path[i + 1].x, m_Path[i + 1].y + 0.2f, m_Path[i + 1].z),
          JPH::Color::sYellow);
    }
  }
}

void EnemyBase::TakeDamage(int damage) {
  spdlog::info("AiNode: Took {} damage", damage);
  m_hp -= damage;
  if (m_hp <= 0) {
    Die();
  }
}