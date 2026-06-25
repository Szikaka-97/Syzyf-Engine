#pragma once

#include <GameObject.h>
#include <MeshRenderer.h>
//#include "GltfImporter.h"
#include "physics/Body.h"
#include "physics/ICollisionReceiver.h"
#include "game_scripts/AttackEffects/EffectsManager.h"
#include <Scene.h>
#include <Resources.h>

class EnemySword : public GameObject, public Physics::ICollisionReceiver {
public:
    SceneNode*     m_PlayerNode = nullptr;
    bool           m_HasHit   = false;
    float          m_HitRadius = 0.7f; // approx half of 1x1 segment
    SceneNode*      myNode     = nullptr;

    EnemySword();

    void Update();
    virtual void OnCollisionEnter(SceneNode* other) override;
    virtual void OnCollisionExit(SceneNode* other) override {};
};
