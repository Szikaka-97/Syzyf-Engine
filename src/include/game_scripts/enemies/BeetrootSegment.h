#pragma once
#include <GameObject.h>
#include <glm/vec3.hpp>
#include "Scene.h"
#include "EnemyBeetroot.h"
//class BeetrootEnemy;

class BeetrootSegment : public GameObject {
private:
    EnemyBeetroot* m_Owner    = nullptr;
    SceneNode*     m_PlayerNode = nullptr;
    bool           m_HasHit   = false;
    float          m_HitRadius = 0.7f; // approx half of 1x1 segment
    SceneNode*      myNode     = nullptr;

public:
    // BeetrootSegment() : GameObject() {
    //     SceneNode* enemyModel =
    //      ResourceDatabase::Global->Get<GltfScene>("./res/models/enemies/burak_segment.glb")
    //          ->Instantiate(GetScene(), GetNode(), "BeetrootSegmentModel");
    //     //enemyModel->SetParent(enemy1);
    //     //enemyModel->GlobalTransform().Scale() = glm::vec3(0.1, 0.1, 0.1);
    //     enemyModel->LocalTransform().Position() = glm::zero<glm::vec3>();
    // };

    void Initialize(EnemyBeetroot* owner, SceneNode* playerNode);

    void Update();
};