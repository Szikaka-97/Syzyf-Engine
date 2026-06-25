#include <./include/game_scripts/enemies/AiSimplified.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <MeshRenderer.h>
#include <TimeSystem.h>
#include "Surface.h" 
#include <physics/LayerMaskFilter.h>

#include "./include/game_scripts/enemies/EnemyBase.h"
#include "./include/game_scripts/enemies/EnemyBullet.h"    

#include "./include/game_scripts/enemies/loot/LootItem.h"

#include "./include/game_scripts/enemies/loot/LootPool.h"

EnemyBase::EnemyBase()
    : fov(glm::radians(180.0f)),
      m_AttackCooldown(1.5f),
      m_AttackTimer(0.0f),
      m_ProjectileSpeed(15.0f),
      m_ProjectileMesh(nullptr),
      m_ProjectileMaterial(nullptr),
      m_hp(100) {
}

EnemyBase::~EnemyBase() {}

void EnemyBase::Awake() {
        if (m_VisualOffset == 0.0f) return;
        for (auto* child : GetNode()->GetChildren()) {
            child->LocalTransform().Position() += glm::vec3(0.f, m_VisualOffset, 0.f);
        }
        physics = GetScene()->GetComponent<Physics::System>();
        
 //flockForce = m_FlockingSystem->GetFlockingForce(this);

    }

void EnemyBase::Attack() {
  m_AttackTimer += Time::Delta();
  if (m_AttackTimer >= m_AttackCooldown) {
    m_AttackTimer            = 0.0f;
    m_InAttackAnimation      = true;
    m_AttackAnimationElapsed = 0.0f;
    m_CurrentAnimation       = "attack.001";  // ustaw bezpośrednio, omijając guard
    PlayAttackAnimation("attack.001");
    SpawnProjectile(m_TargetPosition);
  }
}

void EnemyBase::Die() {

  //spdlog::error("died2");
  if (myNode) {

      std::mt19937 m_Rng{std::random_device{}()};
      int rnd = m_Rng() % 5;
      for (int i = 0; i <= rnd; ++i) {
          DropLoot();
      }

      if (m_FlockingSystem) {
          m_FlockingSystem->Unregister(this);
      }

    //spdlog::error("died");
    GetScene()->QueueDelete(myNode);
    myNode = nullptr;

    if (this->m_Surface) {
      this->m_Surface->RemoveEnemy(this);
    }
  }
}

void EnemyBase::DropLoot() {
    LootPool& pool = GetLootPool();
    const LootItem* item = pool.Draw();
    if (item && myNode) {
        glm::vec3 spawnPos = currentPos + glm::vec3(0.0f, 0.5f, 0.0f);
        item->Spawn(GetScene(), spawnPos);
    }
}
void EnemyBase::SpawnProjectile(const glm::vec3& targetPos) {
  if (!m_ProjectileMesh || !m_ProjectileMaterial) {
    spdlog::warn("AiNode: Projectile resources not set!"+ GetNode()->GetName());
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

  auto* bullet = projectileNode->AddObject<EnemyBullet>();
    bullet->owner = this;
}

void EnemyBase::UpdateAttackAnimation() {
  if (!m_InAttackAnimation) return;
  m_AttackAnimationElapsed += Time::Delta();
  if (m_AttackAnimationElapsed >= m_AttackAnimationDuration) {
    m_InAttackAnimation = false;
    m_CurrentAnimation  = "";
  }
}


void EnemyBase::SetAnimation(const std::string& name) {
  if (!m_AttackAnimation) return;
  if (m_CurrentAnimation == name) return;

  for (auto& anim : m_AttackAnimation->animations) {
    if (anim.data.name == name) {
      anim.timeActive = 0.0f;
      anim.playing    = true;
      anim.looping    = false;
      anim.currentKeyframes.assign(anim.data.tracks.size(), 0);
    } else {
      anim.playing = false;  // ← to samo
    }
  }

  m_CurrentAnimation = name;
  spdlog::debug("EnemyBase: changed animation to {}", name);
}

void EnemyBase::DirectChaseNoBoundary() {
  if (!myNode || !m_Body) return;
  glm::vec3 dir = m_TargetPosition - currentPos;
  dir.y = 0.0f;
  float dist = glm::length(dir);

  if (dist <= attackRange * 0.85f) {
    StopMoving();
    return;
  }
  if (dist < 0.1f) { StopMoving(); return; }

  dir /= dist;

  float speedMultiplier = 1.0f;
  if (dist < attackRange * 1.5f)
    speedMultiplier = 0.5f;

  glm::vec3 newVel = dir * m_Speed * speedMultiplier;
  newVel.y = m_Body->GetLinearVelocity().y;
  m_Body->SetLinearVelocity(newVel);

  float targetYaw = atan2(dir.x, dir.z);
  glm::quat targetRot = glm::angleAxis(targetYaw, glm::vec3(0, 1, 0));
  glm::quat currentRot = myNode->GlobalTransform().Rotation();
  glm::quat newRot = glm::slerp(currentRot, targetRot, m_BossRotationSpeed * Time::Delta());
  m_Body->SetRotation(newRot);
  myNode->GlobalTransform().Rotation() = newRot;
  m_Body->SetAngularVelocity(glm::vec3(0.0f));
}

void EnemyBase::SetLoopingAnimation(const std::string& name) {
  if (!m_AttackAnimation) return;
  if (m_CurrentAnimation == name) return;

  for (auto& anim : m_AttackAnimation->animations) {
    if (anim.data.name == name) {
      anim.timeActive = 0.0f;
      anim.playing    = true;
      anim.looping    = true;
      anim.currentKeyframes.assign(anim.data.tracks.size(), 0);
    } else {
      anim.playing = false;
    }
  }

  m_CurrentAnimation = name;
}

//bool EnemyBase::CanSeePlayer() const {
//  //if (!m_Body) return false;
//  if (!physics) return false;
//
//  glm::vec3 origin = currentPos + glm::vec3(0.0f, 1.0f, 0.0f);
//  glm::vec3 dir = m_TargetPosition - origin;
//  float dist = glm::length(dir);
//  if (dist < 0.1f) return true;
//
//  Physics::LayerMaskFilter filter({}, false);
//  filter.IgnoreBody(m_Body->GetBodyID());
//
//  SceneNode* hit = physics->CastRay(origin, dir * dist, {}, {}, filter).node;
//  if (!hit) return true;
//  return (glm::vec3(hit->GlobalTransform().Position()) == m_TargetPosition);
//}

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
  isPlayerInRoom = false;
}

//void EnemyBase::DrawDebugView() {
//  if (!myNode) return;
//
//  /*auto* scene = GetScene();
//  Physics::DebugRenderer* debugRenderer = scene ?
//  scene->GetComponent<Physics::DebugRenderer>() : nullptr; if (!debugRenderer) {
//          return;
//  }*/
//  if (!myNode) return;
//  auto* debugRenderer =
//      static_cast<Physics::DebugRenderer*>(JPH::DebugRenderer::sInstance);
//  if (!debugRenderer) return;
//
//  if (m_Surface) {
//    m_Surface->DrawDebugSurface(debugRenderer, 0.5f, 1);
//  }
//
//  int segments = 24;
//
//  glm::quat rotation = myNode->GlobalTransform().Rotation();
//  glm::vec3 forward = rotation * glm::vec3(0, 0, 1);
//  forward = glm::normalize(glm::vec3(forward.x, 0, forward.z));
//
//  // glm::vec3 pos = transform;
//
//  std::vector<glm::vec3> arcPoints;
//  float startAngle = atan2(forward.x, forward.z) - fov / 2.0f;
//  for (int i = 0; i <= segments; ++i) {
//    float t = (float)i / segments;
//    float angle = startAngle + t * fov;
//    float x = sightRange * sin(angle);
//    float z = sightRange * cos(angle);
//    arcPoints.push_back(currentPos + glm::vec3(x, 0, z));
//  }
//
//  for (const auto& p : arcPoints) {
//    debugRenderer->DrawLine(JPH::Vec3(currentPos.x, currentPos.y, currentPos.z),
//                            JPH::Vec3(p.x, p.y, p.z), JPH::Color::sPurple);
//  }
//
//  for (size_t i = 0; i < arcPoints.size() - 1; ++i) {
//    debugRenderer->DrawLine(
//        JPH::Vec3(arcPoints[i].x, arcPoints[i].y, arcPoints[i].z),
//        JPH::Vec3(arcPoints[i + 1].x, arcPoints[i + 1].y, arcPoints[i + 1].z),
//        JPH::Color::sPurple);
//  }
//}
//
void EnemyBase::TakeDamage(int damage) {
  spdlog::info("AiNode: Took {} damage", damage);

  if (m_AttackAnimation) {
    m_AttackAnimation->Play("attacked.001");
    m_CurrentAnimation = "attacked.001";
  }

  if (m_hp > 0 && m_hp - damage <= 0) {
    Die();
  }
  m_hp -= damage;
}

void EnemyBase::ApplyBurn(float damagePerTick, float duration, float interval) {
    m_Burn.active        = true;
    m_Burn.remainingTime = duration;
    m_Burn.damage        = damagePerTick;
    m_Burn.interval      = (interval > 0.0f) ? interval : 1.0f;
    m_Burn.intervalTimer = 0.0f;
    spdlog::info("EnemyBase: burn applied ({:.1f} dmg / {:.1f}s, duration {:.1f}s)",
                 damagePerTick, interval, duration);
}
 
void EnemyBase::ApplyPetrify(float slowFactor, float duration) {
    if (m_Petrify.active) return;   
    m_Petrify.active        = true;
    m_Petrify.remainingTime = duration;
    m_Petrify.originalSpeed = GetMovementSpeed();  
    SetMovementSpeed(m_Petrify.originalSpeed * slowFactor);
    spdlog::info("EnemyBase: petrify applied (slowFactor={:.2f}, duration={:.1f}s)",
                 slowFactor, duration);
}
 
void EnemyBase::ApplyConfuse(float duration, bool isPrecise) {
    m_Confuse.active        = true;
    m_Confuse.remainingTime = duration;
    m_Confuse.isPrecise     = isPrecise;
    spdlog::info("EnemyBase: confuse applied (duration={:.1f}s)", duration);
}
 
void EnemyBase::UpdateStatusEffects() {
    const float dt = Time::Delta();
 
    if (m_Petrify.active) {
        m_Petrify.remainingTime -= dt;
        if (m_Petrify.remainingTime <= 0.0f) {
            m_Petrify.active = false;
            SetMovementSpeed(m_Petrify.originalSpeed);
            spdlog::info("EnemyBase: petrify expired, speed restored");
        }
    }
 
  if (m_Burn.active) {
    m_Burn.remainingTime -= dt;
    m_Burn.intervalTimer += dt;

    if (m_Burn.remainingTime <= 0.0f) {
      m_Burn.active = false;
      spdlog::info("EnemyBase: burn expired");
    } else if (m_Burn.intervalTimer >= m_Burn.interval) {
      m_Burn.intervalTimer = 0.0f;

      TakeDamage(static_cast<int>(m_Burn.damage));

      if (m_hp <= 0) return;

      spdlog::debug("EnemyBase: burn tick -{:.0f} hp  (remaining hp={})",
                    m_Burn.damage, m_hp);
    }
  }
 
    if (m_Confuse.active) {
        m_Confuse.remainingTime -= dt;
        if (m_Confuse.remainingTime <= 0.0f) {
            m_Confuse.active = false;
            spdlog::info("EnemyBase: confuse expired");
        }
    }
}

void EnemyBase::ApplyOrbitalVelocity(const glm::vec3& tornadoCenter, float angularSpeedDeg) {
  if (!m_Body || !myNode) return;

  glm::vec3 epos = myNode->GlobalTransform().Position();
  glm::vec3 toEnemy = epos - tornadoCenter;
  toEnemy.y = 0.0f;
  float dist = glm::length(toEnemy);
  if (dist < 0.1f) return;

  glm::vec3 radial  = toEnemy / dist;
  glm::vec3 tangent = glm::vec3(-radial.z, 0.0f, radial.x);

  float linearSpeed = glm::radians(angularSpeedDeg) * dist;
  glm::vec3 vel = tangent * linearSpeed;
  vel.y = m_Body->GetLinearVelocity().y;
  m_Body->SetLinearVelocity(vel);
}

void EnemyBase::DirectChaseWithFlock(const glm::vec3& ) {
    DirectChase();  
}

 
