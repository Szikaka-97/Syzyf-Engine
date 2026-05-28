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
    EnemySword();

    void Update();
    void OnCollisionEnter(SceneNode* other) override;
    void OnCollisionExit(SceneNode* other) override {};
};
