#include "Surface.h"
#include "Mesh.h"
#include "Scene.h"
#include "game_scripts/PlayerController.h"
#include "physics/System.h"
#include <random>
#include <limits>
#include <spdlog/spdlog.h>
//#include "AiNode.h"
#include "./include/game_scripts/enemies/AiSimplified.h"
#include "./include/game_scripts/enemies/EnemyBase.h"
#include <game_scripts/PlayerController.h>

#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <unordered_set>

Surface::Surface(Mesh* floorMesh, float cellSize)
    : floorMesh(floorMesh), cellSize(cellSize), m_playerInside(false) { }

bool Surface::PlayerInside() const {
    return this->m_playerInside;
}

void Surface::Awake() {
    if (!floorMesh || floorMesh->GetSubMeshCount() == 0) {
       // spdlog::error("Surface: No valid mesh provided or mesh has no submeshes");
        return;
    }

    SceneNode* node = GetNode();
    if (!node) {
       // spdlog::error("Surface: Node not found");
        return;
    }

    std::vector<AiSimplified*> enemies = GetScene()->FindObjectsOfType<AiSimplified>();
    for (const auto& enemy : enemies) {
       if(enemy->GetID()== m_RoomID) {
           myEnemies.push_back(enemy);
       }
    }

    CollectVertices();
    spdlog::info("Surface generated {} walkable points", walkablePoints.size());
}

Surface::~Surface() {}

void Surface::CollectVertices() {
    walkablePoints.clear();
    if (!floorMesh) return;

    unsigned int vertexCount = floorMesh->GetVertexCount();
    const float* vertexData = floorMesh->GetVertexData();
    unsigned int stride = floorMesh->GetVertexStride();

    SceneNode* node = GetNode();
    glm::mat4 world = node->GlobalTransform();
    struct IVec3Hash {
    size_t operator()(const glm::ivec3& v) const {
        return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y) << 1) ^ (std::hash<int>()(v.z) << 2);
    }
};
    std::unordered_set<glm::ivec3, IVec3Hash> checkedCells;  
const float cellFilterSize = 1.0f;             

for (unsigned int i = 0; i < vertexCount; ++i) {
    const float* v = vertexData + i * stride;
    glm::vec3 localPos(v[0], v[1], v[2]);
    glm::vec3 worldPos = world * glm::vec4(localPos, 1.0f);

    glm::ivec3 cell = glm::round(worldPos / cellFilterSize);

    if (checkedCells.insert(cell).second) {
        if (!IsPointBlocked(worldPos)) {
            walkablePoints.push_back(worldPos);
        }
    }
}

    CalculateBounds();
}

//void Surface::CalculateBounds() {
//    SceneNode* node = GetNode();
//    if (!node) return;
//    glm::vec3 scale = node->GlobalTransform().Scale();
//    glm::vec3 pos   = node->GlobalTransform().Position();
//    m_size = scale;                
//    m_center = pos + glm::vec3(0, scale.y * 0.5f, 0); 
//}
void Surface::CalculateBounds() {
    if (walkablePoints.empty()) {
        m_center = glm::vec3(0.0f);
        m_size = glm::vec3(0.0f);
        return;
    }
    glm::vec3 minP = walkablePoints[0];
    glm::vec3 maxP = walkablePoints[0];
    for (const auto& p : walkablePoints) {
        minP = glm::min(minP, p);
        maxP = glm::max(maxP, p);
    }
    m_center = (minP + maxP) * 0.5f;
    m_size = maxP - minP;
}

glm::vec3 Surface::GetRandomWalkPoint(const glm::vec3& center, float radius) const {
    if (walkablePoints.empty()) {
        spdlog::error("No walkable points found on surface, returning center point: ({}, {}, {})", center.x, center.y, center.z);
        return center;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, walkablePoints.size() - 1);

    /*for (int attempts = 0; attempts < 20; ++attempts) {
        const auto& candidate = walkablePoints[dist(gen)];
        if (glm::distance(candidate, center) <= radius) {
            return candidate;
        }
    }

    spdlog::warn("No walk point in radius, picking random point from whole list");*/
    return walkablePoints[dist(gen)];
}

float Surface::GetGroundHeight(float x, float z) const {
    auto* physics = GetScene()->GetComponent<Physics::System>();
    if (!physics) return 0.0f;

    JPH::RRayCast ray(JPH::RVec3(x, 500.0f, z), JPH::Vec3(0, -1, 0));
    JPH::RayCastResult result;
    if (physics->GetJoltSystem()->GetNarrowPhaseQuery().CastRay(ray, result)) {
        JPH::RVec3 hit = ray.GetPointOnRay(result.mFraction);
        return static_cast<float>(hit.GetY());
    }
    return 0.0f;
}

void Surface::SetGroundHeight(float height) {
    SceneNode* node = GetNode();
    if (!node) return;
    glm::vec3 pos = node->GlobalTransform().Position();
    pos.y = height;
    node->GlobalTransform().Position() = pos;
}

bool Surface::IsOnSurface(const glm::vec3& point) const {
    auto* physics = GetScene()->GetComponent<Physics::System>();
    if (!physics) return false;

    JPH::RRayCast ray(JPH::RVec3(point.x, point.y, point.z), JPH::Vec3(0, -1, 0));
    JPH::RayCastResult result;
    if (physics->GetJoltSystem()->GetNarrowPhaseQuery().CastRay(ray, result)) {
        return true;
    }
    return false;
}

bool Surface::IsPointBlocked(const glm::vec3& point) const {
    auto* scene = GetScene();
    if (!scene) return false;
    auto* physics = scene->GetComponent<Physics::System>();
    if (!physics) return false;

    glm::vec3 start = point + glm::vec3(0.0f, 0.1f, 0.0f);
    SceneNode* hit = physics->CastRay(start, glm::vec3(0.0f, 1.0f, 0.0f) * 2.0f).node;
    if (hit && hit->GetName() != "Floor")  
        return true;
    return false;
}

bool Surface::ContainsPoint(const glm::vec3& point, float margin) {
    if (walkablePoints.empty()) return false;
    glm::vec3 half = m_size * 0.5f;
    return (point.x >= m_center.x - half.x - margin && point.x <= m_center.x + half.x + margin &&
            point.z >= m_center.z - half.z - margin && point.z <= m_center.z + half.z + margin);
}

void Surface::InformEnter() {
   // spdlog::warn("Player entered surface {}, informing {} enemies", GetID(), myEnemies.size());
    for (auto* enemy : myEnemies) {
        if (enemy) {
            enemy->GetObject<EnemyBase>()->OnPlayerEnteredRoom();
        }
    }
}

void Surface::InformExit() {
   // spdlog::warn("Player exited surface {}, informing {} enemies", GetID(), myEnemies.size());
    for (auto* enemy : myEnemies) {
        if (enemy) {
            enemy->GetObject<EnemyBase>()->OnPlayerExitedRoom();
        }
    }
}

void Surface::DrawDebugSurface(Physics::DebugRenderer* debugRenderer, float pointSize, int step) const {
    if (!debugRenderer) return;
    if (!walkablePoints.empty()) {
        JPH::Color pointColor = JPH::Color::sCyan;
        for (size_t i = 0; i < walkablePoints.size(); i += step) {
            const glm::vec3& p = walkablePoints[i];
            JPH::Vec3 pos(p.x, p.y, p.z);
            debugRenderer->DrawLine(pos + JPH::Vec3(-pointSize, 0, 0), pos + JPH::Vec3(pointSize, 0, 0), pointColor);
            debugRenderer->DrawLine(pos + JPH::Vec3(0, -pointSize, 0), pos + JPH::Vec3(0, pointSize, 0), pointColor);
            debugRenderer->DrawLine(pos + JPH::Vec3(0, 0, -pointSize), pos + JPH::Vec3(0, 0, pointSize), pointColor);
        }
    }

    if (!walkablePoints.empty()) {
        glm::vec3 half = m_size * 0.5f;
        glm::vec3 corners[8] = {
            m_center + glm::vec3(-half.x, -half.y, -half.z),
            m_center + glm::vec3( half.x, -half.y, -half.z),
            m_center + glm::vec3( half.x,  half.y, -half.z),
            m_center + glm::vec3(-half.x,  half.y, -half.z),
            m_center + glm::vec3(-half.x, -half.y,  half.z),
            m_center + glm::vec3( half.x, -half.y,  half.z),
            m_center + glm::vec3( half.x,  half.y,  half.z),
            m_center + glm::vec3(-half.x,  half.y,  half.z)
        };

        JPH::Color boxColor = JPH::Color::sGreen;
        debugRenderer->DrawLine(JPH::Vec3(corners[0].x, corners[0].y, corners[0].z),
                                JPH::Vec3(corners[1].x, corners[1].y, corners[1].z), boxColor);
        debugRenderer->DrawLine(JPH::Vec3(corners[1].x, corners[1].y, corners[1].z),
                                JPH::Vec3(corners[2].x, corners[2].y, corners[2].z), boxColor);
        debugRenderer->DrawLine(JPH::Vec3(corners[2].x, corners[2].y, corners[2].z),
                                JPH::Vec3(corners[3].x, corners[3].y, corners[3].z), boxColor);
        debugRenderer->DrawLine(JPH::Vec3(corners[3].x, corners[3].y, corners[3].z),
                                JPH::Vec3(corners[0].x, corners[0].y, corners[0].z), boxColor);
        // G\F3rna podstawa
        debugRenderer->DrawLine(JPH::Vec3(corners[4].x, corners[4].y, corners[4].z),
                                JPH::Vec3(corners[5].x, corners[5].y, corners[5].z), boxColor);
        debugRenderer->DrawLine(JPH::Vec3(corners[5].x, corners[5].y, corners[5].z),
                                JPH::Vec3(corners[6].x, corners[6].y, corners[6].z), boxColor);
        debugRenderer->DrawLine(JPH::Vec3(corners[6].x, corners[6].y, corners[6].z),
                                JPH::Vec3(corners[7].x, corners[7].y, corners[7].z), boxColor);
        debugRenderer->DrawLine(JPH::Vec3(corners[7].x, corners[7].y, corners[7].z),
                                JPH::Vec3(corners[4].x, corners[4].y, corners[4].z), boxColor);
        // Kraw\EAdzie pionowe
        debugRenderer->DrawLine(JPH::Vec3(corners[0].x, corners[0].y, corners[0].z),
                                JPH::Vec3(corners[4].x, corners[4].y, corners[4].z), boxColor);
        debugRenderer->DrawLine(JPH::Vec3(corners[1].x, corners[1].y, corners[1].z),
                                JPH::Vec3(corners[5].x, corners[5].y, corners[5].z), boxColor);
        debugRenderer->DrawLine(JPH::Vec3(corners[2].x, corners[2].y, corners[2].z),
                                JPH::Vec3(corners[6].x, corners[6].y, corners[6].z), boxColor);
        debugRenderer->DrawLine(JPH::Vec3(corners[3].x, corners[3].y, corners[3].z),
                                JPH::Vec3(corners[7].x, corners[7].y, corners[7].z), boxColor);
    }
    if (!walkablePoints.empty()) {
        JPH::Vec3 center(m_center.x, m_center.y, m_center.z);
        debugRenderer->DrawSphere(center, 0.2f, JPH::Color::sRed);
    }
}

void Surface::Update() {
	PlayerController* player = PlayerController::Instance();

	bool containsPlayer = ContainsPoint(player->GlobalTransform().Position(), 0.2f);
	
	if (m_playerInside && !containsPlayer) {
		m_playerInside = false;
		InformExit();
	}
	else if (!m_playerInside && containsPlayer) {
		m_playerInside = true;

        spdlog::error("Player enter");

		InformEnter();
	}
}

void Surface::DrawImGui() {
    ImGui::LabelText("Enemy count", "%zu", this->myEnemies.size());
}