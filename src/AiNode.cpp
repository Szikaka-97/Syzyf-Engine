#include <AiNode.h>

#include <Scene.h>
#include <TimeSystem.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <Camera.h>
#include <Graphics.h>
#include <imgui.h>
#include <random>
#include <vector>
#include <queue>
#include <unordered_map>
#include <cmath>
#include <algorithm>
#include <limits>
#include <functional>
#include <map>
#include <MeshRenderer.h>
#include "astar/NavigationGrid.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/Character.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Body/BodyFilter.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include "Jolt/Physics/Body/BodyID.h"
#include "Jolt/Physics/Collision/Shape/CapsuleShape.h"
#include "physics/CharacterController.h"
#include "physics/ICollisionReceiver.h"
#include "physics/System.h"
#include "physics/DebugRenderer.h"
#include "physics/Body.h"

#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/CollideShape.h>

AiNode::AiNode()
	: m_Speed(5.0f)
	, m_RotationSpeed(2.0f)
	, m_TargetNode(nullptr)
	, sightRange(15.0f)
	, attackRange(5.0f)
	, walkPointRange(10.0f)
	, walkPointSet(false)
	, m_Body(nullptr)
	, fov(glm::radians(180.0f))
	, m_AttackCooldown(1.5f)        
    , m_AttackTimer(0.0f)
    , m_ProjectileSpeed(15.0f)
    , m_ProjectileMesh(nullptr)
    , m_ProjectileMaterial(nullptr)
	, m_hp(100)
{
	patrolPoints.clear();
	m_Surface = nullptr;
	myNode = GetNode();
	if (myNode) {
		m_Body = myNode->GetObject<Physics::Body>();
		transform = m_Body ? m_Body->GetPosition() : myNode->GlobalTransform().Position();
	}
	walkPoint = glm::vec3(0.0f);
	m_PatrolTimeout = 0.0f;

	 m_Body->SetRestitution(0.0f);
    m_Body->SetFriction(0.5f);
    m_Body->SetLinearDamping(0.1f);
    m_Body->SetCollisionLayerAndMask({ Physics::Layers::MOVING, Physics::Layers::NON_MOVING });

	SetSurface(nullptr);
}

AiNode::~AiNode() {
}

void AiNode::Update() {
	if (!m_TargetNode) {
		Scene* scene = GetScene();
		if (scene) {
			SceneGraphics* graphics = scene->GetGraphics();
			if (graphics) {
				Camera* camera = graphics->GetMainCamera();
				if (camera) {
					m_TargetNode = camera->GetNode();
				}
			}
		}
		if (!m_TargetNode) return;
	}

	if (!m_NavGrid) {
    auto grids = GetScene()->FindObjectsOfType<NavigationGrid>();
    if (!grids.empty()) m_NavGrid = grids[0];
}

	if (!myNode) return;

	transform = m_Body->GetPosition();
	myNode->GlobalTransform().Position() = transform;
	myNode->GlobalTransform().Rotation() = m_Body->GetRotation();
	glm::vec3 targetPos = m_TargetNode->GlobalTransform().Position();
	glm::vec3 dirToTarget = targetPos - transform;
	//float dist = glm::length(dirToTarget);
	//bool canSeePlayer = false;

	//if (dist < sightRange) {
	//	if (dist > 0.001f){
	//		dirToTarget /= dist;
	//		glm::vec3 forward = m_Body->GetRotation() * glm::vec3(0, 0, 1);
	//		forward = glm::normalize(glm::vec3(forward.x, 0, forward.z));
	//		glm::vec3 dirFlat = glm::normalize(glm::vec3(dirToTarget.x, 0, dirToTarget.z));
	//		float dot = glm::dot(forward, dirFlat);
	//		float angle = acos(glm::clamp(dot, -1.0f, 1.0f));
	//		if (angle <= fov / 2.0f) {
	//			canSeePlayer = true;  // bez raycasta ?
	//			//pdlog::info("can see player");
	//		}
	//	}
	//	
	//}

	//bool playerInAttackRange = canSeePlayer && dist < attackRange;

	if (isPlayerInRoom) {
    float dist = glm::distance(transform, targetPos);
    const float keepDist = attackRange;           
    const float tolerance = 0.5f; 
	if (m_hp <= 30) {
		Flee();
		Attack(); 
	}
	else {
		if (dist > keepDist + tolerance) {
        Chase();
    }
    else if (dist < keepDist - tolerance) {
        glm::vec3 awayDir = transform - targetPos;   
        if (glm::length(awayDir) > 0.001f) {
            MoveInDirection(awayDir);
            RotateNode(targetPos - transform);
        }
    }
    else {
        StopMoving();
        Attack();                        
    }
	}
    
} else {
    Patrol();
}

		DrawDebugView();
	}

	void AiNode::SetTarget(SceneNode * target) {
		m_TargetNode = target;
	}

	void AiNode::SetPatrolPoints(const std::vector<glm::vec2>&points) {
		//patrolPoints = points;

		for (const auto& point : points) {
			patrolPoints.push_back(glm::vec3(point.x, m_Surface->GetGroundHeight(point.x, point.y), point.y));
		}

	}

	void AiNode::SetSurface(Surface * surface) {
		if (surface) {
			m_Surface = surface;
		}
		else {
			auto surfaces = GetScene()->FindObjectsOfType<Surface>();
			if (!surfaces.empty()) {
				m_Surface = surfaces[0];
			}
			else {
				spdlog::error("AiNode: No Surface component found in scene");
			}
		}

	}

	void AiNode::SetProjectileResources(Mesh* mesh, Material* material){
		m_ProjectileMesh = mesh;
		m_ProjectileMaterial = material;
	}
	

	void AiNode::SetAttackCooldown(float cooldown){
		m_AttackCooldown = cooldown;
	}

	void AiNode::Patrol() {
		if (!walkPointSet)
		{
			SearchWalkPoint();
		}
		else {
			//glm::vec3 myPos = transform;

			glm::vec3 dir = walkPoint - transform;
			float distance = glm::length(dir);

			m_PatrolTimeout += Time::Delta();
			if (m_PatrolTimeout > 5.0f) {
				spdlog::warn("AiNode: Patrol timeout reached, resetting walk point, current pos ({:.2f},{:.2f},{:.2f}), nex point ({:.2f},{:.2f},{:.2f})", transform.x,transform.y,transform.z, walkPoint.x, walkPoint.y, walkPoint.z);
				walkPointSet = false;
				m_PatrolTimeout = 0.0f;
				return;
			}

			if (distance > 0.5f) {
				 MoveInDirection(dir);
			}
			else {
				StopMoving();
				walkPointSet = false;
			}
		}
	}

	std::vector<glm::vec3> AiNode::FindPath(const glm::vec3& start, const glm::vec3& target) {
		if (!m_Surface) return {};

		glm::vec3 walkableStart = GetNearestWalkable(start, 3.0f);
		glm::vec3 walkableTarget = GetNearestWalkable(target, 3.0f);

		if (!IsWalkable(walkableStart) || !IsWalkable(walkableTarget)) {
			spdlog::warn("FindPath: start or target not walkable even after correction");
			return {};
		}

		spdlog::info("FindPath: using start ({:.2f},{:.2f},{:.2f}) target ({:.2f},{:.2f},{:.2f})",
			walkableStart.x, walkableStart.y, walkableStart.z,
			walkableTarget.x, walkableTarget.y, walkableTarget.z);

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
				return std::hash<int>()(xi) ^ (std::hash<int>()(yi) << 1) ^ (std::hash<int>()(zi) << 2);
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

		auto cmp = [](const Node* a, const Node* b) { return a->fCost() > b->fCost(); };
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

				float tentativeGCost = current->gCost + glm::distance(current->pos, neighborPos);
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

	float AiNode::Heuristic(const glm::vec3 & a, const glm::vec3 & b) {
		glm::vec3 diff = a - b;
		float distance = glm::length(diff);
		return distance;
	}

	bool AiNode::IsWalkable(const glm::vec3 & point) {

		/*if (!m_Surface) return false;
		float groundY = m_Surface->GetGroundHeight(point.x, point.z);
		if (std::abs(point.y - groundY) > 0.5f) return false; 

		auto* physics = GetScene()->GetComponent<Physics::System>();
		if (physics) {
			JPH::RRayCast ray(JPH::RVec3(point.x, point.y + 2.0f, point.z), JPH::Vec3(0, -1, 0));
			JPH::RayCastResult result;
			if (!physics->GetSystem().GetNarrowPhaseQuery().CastRay(ray, result)) return false;
		}
		return true;*/
		if(!m_Surface) return false;
		float groundY = m_Surface->GetGroundHeight(point.x, point.z);
		if (std::abs(point.y - groundY) > 0.5f) return false;
		return true;  // bez raycasta
	}

	glm::vec3 AiNode::GetNearestWalkable(const glm::vec3& point, float radius) {
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
		return point; // fallback – oryginalny punkt
	}

	std::vector<glm::vec3> AiNode::GetNeighbors(const glm::vec3 & node) {
		if (!m_Surface) return {};
		std::vector<glm::vec3> neighbors;
		const float step = 1.0f;
		//const float step = 0.5f;
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

	void AiNode::AstarChase() {
    if (!m_NavGrid || !m_TargetNode) return;
    
    glm::vec3 targetPos = m_TargetNode->GlobalTransform().Position();

    m_ChasePathUpdateTimer += Time::Delta();
    if (m_ChasePathUpdateTimer > 0.5f || m_Path.empty()) {
        m_Path = m_NavGrid->FindPath(transform, targetPos);
        m_CurrentPathIndex = 0;
        m_ChasePathUpdateTimer = 0.0f;
    }

    if (!m_Path.empty() && m_CurrentPathIndex < m_Path.size()) {
        glm::vec3 next = m_Path[m_CurrentPathIndex];
        if (glm::distance(transform, next) < 0.5f) {
            m_CurrentPathIndex++;
        } else {
            MoveInDirection(next - transform);
        }
    }
}
	void AiNode::Chase() {
		glm::vec3 targetPos = m_TargetNode->GlobalTransform().Position();
    glm::vec3 dir = targetPos - transform;
    float distance = glm::length(dir);
    if (distance != attackRange) {
       MoveInDirection(dir);
    }
    else {
        StopMoving();
    }
	}

	void AiNode::Attack() {
    if (!m_TargetNode) return;
	
	glm::vec3 targetPos = m_TargetNode->GlobalTransform().Position();
	glm::vec3 dirTo = targetPos - transform;
        if (glm::length(dirTo) > 0.01f)
            RotateNode(dirTo);

    m_AttackTimer += Time::Delta();
    if (m_AttackTimer >= m_AttackCooldown) {
        m_AttackTimer = 0.0f;

        glm::vec3 targetPos = m_TargetNode->GlobalTransform().Position();
		PlayAttackAnimation();
        SpawnProjectile(targetPos);
    }
}
void AiNode::SpawnProjectile(const glm::vec3& targetPos) {
    if (!m_ProjectileMesh || !m_ProjectileMaterial) {
        spdlog::warn("AiNode: Projectile resources not set!");
        return;
    }

    glm::vec3 startPos = transform + glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 dir = glm::normalize(targetPos - startPos);

    auto* projectileNode = GetScene()->CreateNode("EnemyProjectile");
    projectileNode->AddObject<MeshRenderer>(m_ProjectileMesh, m_ProjectileMaterial); 
    projectileNode->GlobalTransform().Position() = startPos;
    projectileNode->GlobalTransform().Scale() = glm::vec3(0.2f);

    JPH::BodyCreationSettings projectileSettings(
        new JPH::SphereShape(0.2f),
        JPH::RVec3(startPos.x, startPos.y, startPos.z),
        JPH::Quat::sIdentity(),
        JPH::EMotionType::Dynamic,
        Physics::Layers::MOVING
    );

    auto* body = projectileNode->AddObject<Physics::Body>(projectileSettings);
    
    JPH::Vec3 jphVel = JPH::Vec3(dir.x, dir.y, dir.z) * m_ProjectileSpeed;
    body->SetLinearVelocity(glm::vec3(jphVel.GetX(), jphVel.GetY(), jphVel.GetZ()));
    
    body->SetRestitution(0.3f);
    body->SetFriction(0.5f);
   // body->Awake();
}

void AiNode::SetAttackAnimation(AnimationComponent* anim) {
    m_AttackAnimation = anim;
}

void AiNode::PlayAttackAnimation() {
    if (m_AttackAnimation) {
        m_AttackAnimation->Play("attack.001");   // nazwa animacji – dostosuj do swojego modelu
    }
}

void AiNode::TakeDamage(int damage) {
	spdlog::info("AiNode: Took {} damage", damage);
	m_hp -= damage;
	if (m_hp <= 0) {
		Die();
	}
}

void AiNode::Die() {
	if (myNode) {
        GetScene()->QueueDelete(myNode);
        myNode = nullptr;
    }
}

void AiNode::Flee() {
    if (m_hp > 30) return;
    glm::vec3 targetPos = m_TargetNode->GlobalTransform().Position();
    glm::vec3 dirAway = transform - targetPos;
    if (glm::length(dirAway) < 0.1f) {
        StopMoving();
        return;
    }
    dirAway = glm::normalize(dirAway);
    glm::vec3 fleeTarget = transform + dirAway * 10.0f; 
	///opt  AstarChase do ruchu w kierunku fleeTarget
    MoveInDirection(dirAway);
}

void AiNode::MoveInDirection(const glm::vec3& direction) {
    if (glm::length(direction) < 0.001f) {
        StopMoving();
        return;
    }
    glm::vec3 dir = glm::normalize(direction);
    glm::vec3 currentVel = m_Body->GetLinearVelocity();
    glm::vec3 newVel = dir * m_Speed;
    newVel.y = currentVel.y;

	if (m_Surface) {
        glm::vec3 predictedPos = transform + newVel * Time::Delta();
        if (!m_Surface->ContainsPoint(predictedPos,0.2)) {
            StopMoving(); 
            return;
        }
    }

    m_Body->SetLinearVelocity(newVel);
    RotateNode(dir);
}

void AiNode::StopMoving() {
    glm::vec3 currentVel = m_Body->GetLinearVelocity();
    m_Body->SetLinearVelocity(glm::vec3(0, currentVel.y, 0));
}

	void AiNode::RotateNode(glm::vec3 dir) {
		if (glm::length(dir) > 0.01f) {
			dir = glm::normalize(dir);
			float targetYaw = atan2(dir.x, dir.z);
			glm::quat targetRot = glm::angleAxis(targetYaw, glm::vec3(0, 1, 0));
			glm::quat currentRot = myNode->GlobalTransform().Rotation();
			glm::quat newRot = glm::slerp(currentRot, targetRot, m_RotationSpeed * Time::Delta());
			m_Body->SetRotation(newRot);
			myNode->GlobalTransform().Rotation() = newRot;
			m_Body->SetAngularVelocity(glm::vec3(0, 0, 0));
		}
	}

void AiNode::OnPlayerEnteredRoom() {
		isPlayerInRoom=true;
}

void AiNode::OnPlayerExitedRoom() {
		isPlayerInRoom=false;
	}

void AiNode::SearchWalkPoint() {
		if (m_Surface) {
			if (patrolPoints.size() > 0) {
				LookForNextPoint();
			}
			else {
				if (m_NavGrid && m_NavGrid->IsBuilt()) {
    //walkPoint = m_NavGrid->GetRandomWalkablePosition(transform, walkPointRange);
					walkPoint = m_NavGrid->GetRandomWalkablePosition(transform, 1000.0f);
					walkPointSet = true;
				}
				else {
					 float radius = glm::length(m_Surface->GetSize()) * 0.5f;
                walkPoint = m_Surface->GetRandomWalkPoint(m_Surface->GetCenter(), radius);
				spdlog::warn("AiNode: NavGrid not available, using random walk point on surface ({:.2f},{:.2f},{:.2f})", walkPoint.x,walkPoint.y,walkPoint.z);
                walkPointSet = true;
				}
			}
		}
		else {
			spdlog::error("AiNode: Cannot search walk point - no Surface reference");
		}
	}


	void AiNode::LookForNextPoint() {
		posIndex++;
		if (posIndex == patrolPoints.size()) {
			posIndex = 0;
		}
		walkPoint = patrolPoints[posIndex];
		walkPointSet = true;
	}
	void AiNode::DrawDebugView() {
		if (!myNode) return;

		/*auto* scene = GetScene();
		Physics::DebugRenderer* debugRenderer = scene ? scene->GetComponent<Physics::DebugRenderer>() : nullptr;
		if (!debugRenderer) {
			return;
		}*/
		if (!myNode) return;
    auto* debugRenderer = static_cast<Physics::DebugRenderer*>(JPH::DebugRenderer::sInstance);
    if (!debugRenderer) return;


		if (m_Surface) {
        m_Surface->DrawDebugSurface(debugRenderer, 0.5f, 1);
    }

		int segments = 24;

		glm::quat rotation = myNode->GlobalTransform().Rotation();
		glm::vec3 forward = rotation * glm::vec3(0, 0, 1);
		forward = glm::normalize(glm::vec3(forward.x, 0, forward.z));

		//glm::vec3 pos = transform;

		std::vector<glm::vec3> arcPoints;
		float startAngle = atan2(forward.x, forward.z) - fov / 2.0f;
		for (int i = 0; i <= segments; ++i) {
			float t = (float)i / segments;
			float angle = startAngle + t * fov;
			float x = sightRange * sin(angle);
			float z = sightRange * cos(angle);
			arcPoints.push_back(transform + glm::vec3(x, 0, z));
		}

		for (const auto& p : arcPoints) {
			debugRenderer->DrawLine(JPH::Vec3(transform.x, transform.y, transform.z), JPH::Vec3(p.x, p.y, p.z), JPH::Color::sPurple);
		}

		for (size_t i = 0; i < arcPoints.size() - 1; ++i) {
			debugRenderer->DrawLine(JPH::Vec3(arcPoints[i].x, arcPoints[i].y, arcPoints[i].z),
				JPH::Vec3(arcPoints[i + 1].x, arcPoints[i + 1].y, arcPoints[i + 1].z),
				JPH::Color::sPurple);
		}
		if (m_Path.size() >= 2) {
			for (size_t i = 0; i < m_Path.size() - 1; ++i) {
				debugRenderer->DrawLine(JPH::Vec3(m_Path[i].x, m_Path[i].y + 0.2f, m_Path[i].z),
					JPH::Vec3(m_Path[i + 1].x, m_Path[i + 1].y + 0.2f, m_Path[i + 1].z),
					JPH::Color::sYellow);
			}
		}
	}
