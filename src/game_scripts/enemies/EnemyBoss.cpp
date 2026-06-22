#include <./include/game_scripts/enemies/EnemyBoss.h>
#include <TimeSystem.h>

void EnemyBoss::StartAttack() {
  m_IsAttacking         = true;
  StopMoving();


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

void EnemyBoss::Update() {

  EnsureBody();
  LockXZRotation();
  if (!m_TargetNode) return;

  m_TargetPosition = m_TargetNode->GlobalTransform().Position();
  if (!myNode) return;

  currentPos = m_Body->GetPosition();
  myNode->GlobalTransform().Position() = currentPos;
  myNode->GlobalTransform().Rotation() = m_Body->GetRotation();

  UpdateAttackAnimation();
  if (m_InAttackAnimation) { StopMoving(); return; }

  if (isPlayerInRoom) {
    float dist = glm::distance(currentPos, m_TargetPosition);
     if (dist <= attackRange) currentState = States::ATTACKING;
    else                          currentState = States::CHASING;
  } else {
    currentState = States::PATROLLING;
  }
  switch (currentState) {
  case States::PATROLLING:
    Patrol();
    break;
  case States::CHASING:
    DirectChase();
    break;
  case States::ATTACKING:
    if (!m_IsAttacking && m_AttackCooldown <= 0.0f)
      StartAttack();
    else {
      StopMoving();
      m_AttackCooldown -= Time::Delta();
    }
    break;

  }

  //DrawDebugView();
}
