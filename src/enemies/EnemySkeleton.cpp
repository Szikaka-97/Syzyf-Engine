#include <enemies/EnemyBase.h>
#include <AiSimplified.h>
#include <glm/glm.hpp>

class EnemySkeleton : public EnemyBase {
	public:
		void Update() {
			/*if (m_TargetPosition==nullptr) {
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
	}*/
			 EnsureBody();
	if (!m_NavGrid) {
    auto grids = GetScene()->FindObjectsOfType<NavigationGrid>();
    if (!grids.empty()) m_NavGrid = grids[0];
}

	if (!myNode) return;

	currentPos = m_Body->GetPosition();
	myNode->GlobalTransform().Position() = currentPos;
	myNode->GlobalTransform().Rotation() = m_Body->GetRotation();
	glm::vec3 dirToTarget = m_TargetPosition - currentPos;
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
	UpdateAttackAnimation();

if (m_InAttackAnimation) {
    StopMoving();
    return;
}
if (isPlayerInRoom) {
    UpdateStuckDetection();
}
	if (isPlayerInRoom) {
    float dist = glm::distance(currentPos, m_TargetPosition);
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
        glm::vec3 awayDir = currentPos - m_TargetPosition;   
        if (glm::length(awayDir) > 0.001f) {
            MoveInDirection(awayDir);
            RotateNode(m_TargetPosition - currentPos);
        }
    }
    else {
        StopMoving();
        Attack();                        
    }
	}
    
} else {
		m_UsingAStar = false;
m_Path.clear();
m_StuckTimer = 0.0f;
    Patrol();
}

		DrawDebugView();
		}
	};

//void AiSimplified::Update() {
//	//if (!m_TargetPosition) return;
//	if (!myNode) return;
//	currentPos = myNode->GlobalTransform().Position();
//	glm::vec3 dirToTarget = m_TargetPosition - currentPos;
//	float distance = glm::length(dirToTarget);
//	if (distance < 0.1f) {
//		StopMoving();
//		return;
//	}
//	dirToTarget /= distance; // normalize
//	MoveInDirection(dirToTarget);
//}

//void EnemyBase::Update() {
//	UpdateAttackAnimation();
//	if (isPlayerInRoom) {
//    UpdateStuckDetection();
//}
//	if (isPlayerInRoom) {
//    float dist = glm::distance(currentPos, m_TargetPosition);
//    const float keepDist = attackRange;           
//    const float tolerance = 0.5f; 
//	if (m_hp <= 30) {
//		Flee();
//		Attack(); 
//	}
//	else {
//		if (dist > keepDist + tolerance) {
//        Chase();
//    }
//    else if (dist < keepDist - tolerance) {
//        glm::vec3 awayDir = currentPos - m_TargetPosition;   
//        if (glm::length(awayDir) > 0.001f) {
//            MoveInDirection(awayDir);
//            RotateNode(m_TargetPosition - currentPos);
//        }
//    }
//    else {
//        StopMoving();
//        Attack();                        
//    }
//	}
//    
//} else {
//		m_UsingAStar = false;
//m_Path.clear();
//m_StuckTimer = 0.0f;
//    Patrol();
//}
//	}