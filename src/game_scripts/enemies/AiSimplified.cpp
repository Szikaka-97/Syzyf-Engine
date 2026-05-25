#include <./include/game_scripts/enemies/AiSimplified.h>
#include <Camera.h>
#include <Graphics.h>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyFilter.h>
#include <Jolt/Physics/Character/Character.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <MeshRenderer.h>
#include <Scene.h>
#include <TimeSystem.h>
#include <imgui.h>
#include <physics/LayerMaskFilter.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <limits>
#include <map>
#include <queue>
#include <random>
#include <unordered_map>
#include <vector>

#include "Jolt/Physics/Body/BodyID.h"
#include "Jolt/Physics/Collision/Shape/CapsuleShape.h"
#include "astar/NavigationGrid.h"
#include "physics/Body.h"
#include "physics/CharacterController.h"
#include "physics/DebugRenderer.h"
#include "physics/ICollisionReceiver.h"
#include "physics/System.h"
#include "physics/Water.h"

AiSimplified::AiSimplified()
    : m_Speed(5.0f), m_RotationSpeed(2.0f), m_TargetPosition(0.0f) {
  m_Body = nullptr;
  m_Surface = nullptr;
  myNode = GetNode();
  // m_Body = myNode->GetObject<Physics::Body>();
}

AiSimplified::~AiSimplified() {}

void AiSimplified::EnsureBody() {
    if (myNode == nullptr) {
        myNode = GetNode();
    }
    if (myNode != nullptr && m_Body == nullptr) {
        m_Body = myNode->GetObject<Physics::Body>();

        if (m_Body) {
            m_Body->SetAngularDamping(999.0f); 
        }
    }
}

void AiSimplified::LockXZRotation() {
    if (!m_Body) return;

    glm::quat rot = m_Body->GetRotation();

    const float epsilon = 0.001f;
    if (glm::abs(rot.x) > epsilon || glm::abs(rot.z) > epsilon) {
        glm::quat corrected = glm::quat(rot.w, 0.0f, rot.y, 0.0f);
        corrected = glm::normalize(corrected);

        m_Body->SetRotation(corrected);
        myNode->GlobalTransform().Rotation() = corrected;

        glm::vec3 angVel = m_Body->GetAngularVelocity();
        m_Body->SetAngularVelocity(glm::vec3(0.0f, angVel.y, 0.0f));
    }
}

void AiSimplified::MoveInDirection(const glm::vec3& direction) {
  EnsureBody();
  if (glm::length(direction) < 0.001f) {
    StopMoving();
    return;
  }
  glm::vec3 dir = glm::normalize(direction);
  glm::vec3 currentVel = m_Body->GetLinearVelocity();
  glm::vec3 newVel = dir * m_Speed;
  newVel.y = currentVel.y;

  if (m_Surface) {
    glm::vec3 predictedPos = currentPos + newVel * Time::Delta();
    if (!m_Surface->ContainsPoint(predictedPos, 0.2)) {
      StopMoving();
      return;
    }
  }

  m_Body->SetLinearVelocity(newVel);
  RotateNode(dir);
}

void AiSimplified::RotateNode(glm::vec3 dir) {
  EnsureBody();
  if (glm::length(dir) > 0.01f) {
    dir = glm::normalize(dir);
    float targetYaw = atan2(dir.x, dir.z);
    glm::quat targetRot = glm::angleAxis(targetYaw, glm::vec3(0, 1, 0));
    glm::quat currentRot = myNode->GlobalTransform().Rotation();
    glm::quat newRot =
        glm::slerp(currentRot, targetRot, m_RotationSpeed * Time::Delta());
    m_Body->SetRotation(newRot);
    myNode->GlobalTransform().Rotation() = newRot;
    m_Body->SetAngularVelocity(glm::vec3(0, 0, 0));
  }
}

void AiSimplified::SetTarget(glm::vec3 target) { m_TargetPosition = target; }

void AiSimplified::SetSurface(Surface* surface) {
  if (surface) {
    m_Surface = surface;
  } else {
    auto surfaces = GetScene()->FindObjectsOfType<Surface>();
    if (!surfaces.empty()) {
      m_Surface = surfaces[0];
    } else {
      spdlog::error("AiNode: No Surface component found in scene");
    }
  }
}

void AiSimplified::UpdateStuckDetection() {
  if (m_UsingAStar) return;

  float dist = glm::distance(currentPos, glm::vec3(m_TargetPosition));
  // if (dist <= attackRange + 0.5f) return;

  float moved = glm::distance(currentPos, m_LastChasePosition);
  if (moved < m_MinMovementThreshold) {
    m_StuckTimer += Time::Delta();
    spdlog::debug("AiNode stuck timer: {:.2f}s (moved {:.3f})", m_StuckTimer,
                  moved);
  } else {
    m_StuckTimer = 0.0f;
    m_LastChasePosition = currentPos;
  }

  if (m_StuckTimer > m_StuckThreshold) {
    if (m_NavGrid && m_NavGrid->IsBuilt()) {
      spdlog::warn("AiNode stuck! Switching to A* pathfinding.");
      m_Path = m_NavGrid->FindPath(currentPos, m_TargetPosition);
      if (!m_Path.empty()) {
        m_CurrentPathIndex = 0;
        m_UsingAStar = true;
        m_ChasePathUpdateTimer = 0.0f;
        m_StuckTimer = 0.0f;
      } else {
        spdlog::warn("AiNode: A* found no path, continuing direct chase.");
      }
    }
    m_StuckTimer = 0.0f;
  }
}

void AiSimplified::Patrol() {
  if (!walkPointSet) {
    SearchWalkPoint();
  } else {
    // glm::vec3 myPos = transform;

    glm::vec3 dir = walkPoint - currentPos;
    float distance = glm::length(dir);

    m_PatrolTimeout += Time::Delta();
    if (m_PatrolTimeout > 5.0f) {
      spdlog::warn(
          "AiNode: Patrol timeout reached, resetting walk point, current pos "
          "({:.2f},{:.2f},{:.2f}), nex point ({:.2f},{:.2f},{:.2f})",
          m_TargetPosition.x, m_TargetPosition.y, m_TargetPosition.z,
          walkPoint.x, walkPoint.y, walkPoint.z);
      walkPointSet = false;
      m_PatrolTimeout = 0.0f;
      return;
    }

    if (distance > 0.5f) {
      MoveInDirection(dir);
    } else {
      StopMoving();
      walkPointSet = false;
    }
  }
}

void AiSimplified::Chase() {
  if (m_UsingAStar && !m_Path.empty() && m_CurrentPathIndex < m_Path.size()) {
    m_ChasePathUpdateTimer += Time::Delta();

    if (m_ChasePathUpdateTimer > 0.5f) {
      m_ChasePathUpdateTimer = 0.0f;

      /*if (CanSeePlayer()) {
          spdlog::info("AiNode: Player visible, abandoning A*");
          m_UsingAStar = false;
          m_Path.clear();
      }
      else {
          glm::vec3 targetPos = m_TargetNode->GlobalTransform().Position();
          auto newPath = m_NavGrid->FindPath(transform, targetPos);
          if (!newPath.empty()) {
              m_Path = std::move(newPath);
              m_CurrentPathIndex = 0;
              spdlog::debug("AiNode: A* path refreshed, {} nodes",
      m_Path.size()); } else { spdlog::warn("AiNode: Failed to refresh A* path,
      reverting to direct chase"); m_UsingAStar = false; m_Path.clear();
          }
      }*/
    }

    if (m_UsingAStar) {
      glm::vec3 nextPoint = m_Path[m_CurrentPathIndex];
      float distToNext = glm::distance(m_TargetPosition, nextPoint);
      if (distToNext < 0.5f) {
        m_CurrentPathIndex++;
        if (m_CurrentPathIndex >= m_Path.size()) {
          m_UsingAStar = false;
          m_Path.clear();
          spdlog::info("AiNode: A* path completed, back to direct chase.");
        }
      } else {
        MoveInDirection(nextPoint - m_TargetPosition);
      }
      return;
    }
  }

  glm::vec3 dir = m_TargetPosition - currentPos;
  float distance = glm::length(dir);
  if (distance > 0.1f) {
    MoveInDirection(dir);
  } else {
    StopMoving();
  }
}

void AiSimplified::DirectChase() {
  EnsureBody();
  glm::vec3 dir = m_TargetPosition - currentPos;
  float distance = glm::length(dir);
  if (distance > 0.1f) {
    dir /= distance;
    dir = ComputeSteeringDirection(dir, m_Speed);
    MoveInDirection(dir);
  } else {
    StopMoving();
  }
}

void AiSimplified::AstarChase() {
  if (!m_NavGrid) return;

  m_ChasePathUpdateTimer += Time::Delta();
  if (m_ChasePathUpdateTimer > 0.5f || m_Path.empty()) {
    m_Path = m_NavGrid->FindPath(currentPos, m_TargetPosition);
    m_CurrentPathIndex = 0;
    m_ChasePathUpdateTimer = 0.0f;
  }

  if (!m_Path.empty() && m_CurrentPathIndex < m_Path.size()) {
    glm::vec3 next = m_Path[m_CurrentPathIndex];
    if (glm::distance(currentPos, next) < 0.5f) {
      m_CurrentPathIndex++;
    } else {
      MoveInDirection(next - currentPos);
    }
  }
}

void AiSimplified::Flee() {  // if (m_hp > 30) return;
  glm::vec3 dirAway = currentPos - m_TargetPosition;
  if (glm::length(dirAway) < 0.1f) {
    StopMoving();
    return;
  }
  dirAway = glm::normalize(dirAway);
  glm::vec3 fleeTarget = currentPos + dirAway * 10.0f;
  /// opt  AstarChase do ruchu w kierunku fleeTarget
  MoveInDirection(dirAway);
}
void AiSimplified::StopMoving() {
  EnsureBody();
  glm::vec3 currentVel = m_Body->GetLinearVelocity();
  m_Body->SetLinearVelocity(glm::vec3(0, currentVel.y, 0));
  /*if (!m_InAttackAnimation) {
  SetAnimation("stop.001");
}*/
}

void AiSimplified::SearchWalkPoint() {
  if (m_Surface) {
    if (patrolPoints.size() > 0) {
      LookForNextPoint();
    } else {
      if (m_NavGrid && m_NavGrid->IsBuilt()) {
        // walkPoint = m_NavGrid->GetRandomWalkablePosition(transform,
        // walkPointRange);
        walkPoint = m_NavGrid->GetRandomWalkablePosition(currentPos, 1000.0f);
        walkPointSet = true;
      } else {
        float radius = glm::length(m_Surface->GetSize()) * 0.5f;
        walkPoint =
            m_Surface->GetRandomWalkPoint(m_Surface->GetCenter(), radius);
        spdlog::warn(
            "AiNode: NavGrid not available, using random walk point on surface "
            "({:.2f},{:.2f},{:.2f})",
            walkPoint.x, walkPoint.y, walkPoint.z);
        walkPointSet = true;
      }
    }
  } else {
    spdlog::error("AiNode: Cannot search walk point - no Surface reference");
  }
}

void AiSimplified::LookForNextPoint() {
  posIndex++;
  if (posIndex == patrolPoints.size()) {
    posIndex = 0;
  }
  walkPoint = patrolPoints[posIndex];
  walkPointSet = true;
}

std::vector<glm::vec3> AiSimplified::FindPath(const glm::vec3& start,
                                              const glm::vec3& target) {
  if (!m_Surface) return {};

  glm::vec3 walkableStart = GetNearestWalkable(start, 3.0f);
  glm::vec3 walkableTarget = GetNearestWalkable(target, 3.0f);

  if (!IsWalkable(walkableStart) || !IsWalkable(walkableTarget)) {
    spdlog::warn(
        "FindPath: start or target not walkable even after correction");
    return {};
  }

  spdlog::info(
      "FindPath: using start ({:.2f},{:.2f},{:.2f}) target "
      "({:.2f},{:.2f},{:.2f})",
      walkableStart.x, walkableStart.y, walkableStart.z, walkableTarget.x,
      walkableTarget.y, walkableTarget.z);

  struct Node {
    glm::vec3 pos;
    float gCost = 0.0f;
    float hCost = 0.0f;
    Node* parent = nullptr;
    bool closed = false;
    float fCost() const { return gCost + hCost; }
  };

  struct Vec3Hash {
    size_t operator()(const glm::vec3& v) const {
      int xi = static_cast<int>(std::round(v.x * 10.0f));
      int yi = static_cast<int>(std::round(v.y * 10.0f));
      int zi = static_cast<int>(std::round(v.z * 10.0f));
      return std::hash<int>()(xi) ^ (std::hash<int>()(yi) << 1) ^
             (std::hash<int>()(zi) << 2);
    }
  };
  struct Vec3Equal {
    bool operator()(const glm::vec3& a, const glm::vec3& b) const {
      return glm::distance(a, b) < 0.05f;
    }
  };
  std::unordered_map<glm::vec3, Node, Vec3Hash, Vec3Equal> allNodes;

  auto getNode = [&](const glm::vec3& p) -> Node& {
    auto it = allNodes.find(p);
    if (it != allNodes.end()) return it->second;
    Node n;
    n.pos = p;
    n.gCost = std::numeric_limits<float>::max();
    allNodes[p] = n;
    return allNodes[p];
  };

  Node& startNode = getNode(walkableStart);
  startNode.gCost = 0;
  startNode.hCost = Heuristic(walkableStart, walkableTarget);

  auto cmp = [](const Node* a, const Node* b) {
    return a->fCost() > b->fCost();
  };
  std::priority_queue<Node*, std::vector<Node*>, decltype(cmp)> openSet(cmp);
  openSet.push(&startNode);

  const int maxIter = 5000;
  int iter = 0;
  while (!openSet.empty() && iter++ < maxIter) {
    Node* current = openSet.top();
    openSet.pop();

    if (current->closed) continue;
    current->closed = true;

    if (Heuristic(current->pos, walkableTarget) < 0.5f) {
      std::vector<glm::vec3> path;
      Node* node = current;
      while (node) {
        path.push_back(node->pos);
        node = node->parent;
      }
      std::reverse(path.begin(), path.end());
      spdlog::info("FindPath: path found with {} nodes", path.size());
      return path;
    }

    for (const auto& neighborPos : GetNeighbors(current->pos)) {
      Node& neighbor = getNode(neighborPos);
      if (neighbor.closed) continue;

      float tentativeGCost =
          current->gCost + glm::distance(current->pos, neighborPos);
      if (tentativeGCost < neighbor.gCost) {
        neighbor.parent = current;
        neighbor.gCost = tentativeGCost;
        neighbor.hCost = Heuristic(neighborPos, walkableTarget);
        openSet.push(&neighbor);
      }
    }
  }

  spdlog::warn("FindPath: no path found after {} iterations", iter);
  return {};
}

std::vector<glm::vec3> AiSimplified::GetNeighbors(const glm::vec3& node) {
  if (!m_Surface) return {};
  std::vector<glm::vec3> neighbors;
  const float step = 1.0f;
  // const float step = 0.5f;
  for (int dx = -1; dx <= 1; ++dx) {
    for (int dz = -1; dz <= 1; ++dz) {
      if (dx == 0 && dz == 0) continue;
      glm::vec3 candidate(node.x + dx * step, node.y, node.z + dz * step);
      candidate.y = m_Surface->GetGroundHeight(candidate.x, candidate.z);
      if (IsWalkable(candidate)) {
        neighbors.push_back(candidate);
      }
    }
  }
  return neighbors;
}

float AiSimplified::Heuristic(const glm::vec3& a, const glm::vec3& b) {
  glm::vec3 diff = a - b;
  float distance = glm::length(diff);
  return distance;
}

bool AiSimplified::IsWalkable(const glm::vec3& point) {
  if (!m_Surface) return false;
  float groundY = m_Surface->GetGroundHeight(point.x, point.z);
  if (std::abs(point.y - groundY) > 0.5f) return false;
  return true;
}

glm::vec3 AiSimplified::GetNearestWalkable(const glm::vec3& point,
                                           float radius) {
  if (!m_Surface) return point;
  if (IsWalkable(point)) return point;

  const float step = 0.5f;
  for (float r = step; r <= radius; r += step) {
    for (int dx = -1; dx <= 1; ++dx) {
      for (int dz = -1; dz <= 1; ++dz) {
        if (dx == 0 && dz == 0) continue;
        glm::vec3 candidate(point.x + dx * r, point.y, point.z + dz * r);
        candidate.y = m_Surface->GetGroundHeight(candidate.x, candidate.z);
        if (IsWalkable(candidate)) {
          return candidate;
        }
      }
    }
  }
  return point;
}

void AiSimplified::SetPatrolPoints(const std::vector<glm::vec2>& points) {
  for (const auto& point : points) {
    patrolPoints.push_back(glm::vec3(
        point.x, m_Surface->GetGroundHeight(point.x, point.y), point.y));
  }
}

glm::vec3 AiSimplified::ComputeSteeringDirection(glm::vec3 desiredDir,
                                                 float speed) {
  auto* physics = GetScene()->GetComponent<Physics::System>();
  if (!physics) return desiredDir;

  EnsureBody();
  glm::vec3 origin = currentPos + glm::vec3(0.0f, 1.0f, 0.0f);
  glm::vec3 dir = glm::normalize(desiredDir);
  float lookAhead = 1.0f + speed * 0.4f;

  JPH::ShapeRefC shape = new JPH::SphereShape(m_AvoidanceRadius);

  Physics::LayerMaskFilter filter({}, false);
  if (m_Body) filter.IgnoreBody(m_Body->GetBodyID());

  std::vector<SceneNode*> hits =
      physics->CastShape(origin, dir * lookAhead, shape, {}, {}, filter);

  hits.erase(
      std::remove_if(hits.begin(), hits.end(),
                     [](SceneNode* n) { return n && n->GetName() == "Floor"; }),
      hits.end());

  if (!hits.empty()) {
    glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::normalize(glm::cross(dir, worldUp));
    if (glm::length(right) < 0.01f) right = glm::vec3(1.0f, 0.0f, 0.0f);

    std::vector<SceneNode*> rightHits = physics->CastShape(
        origin, right * m_AvoidanceRadius * 2.0f, shape, {}, {}, filter);
    rightHits.erase(std::remove_if(rightHits.begin(), rightHits.end(),
                                   [](SceneNode* n) {
                                     return n && n->GetName() == "Floor";
                                   }),
                    rightHits.end());

    if (!rightHits.empty()) {
      right = -right;
    }

    glm::vec3 steer = desiredDir + right * m_AvoidanceWeight;
    if (glm::length(steer) < 0.01f) return desiredDir;
    return glm::normalize(steer);
  }

  return desiredDir;
}