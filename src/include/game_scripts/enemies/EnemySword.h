#pragma once

#include <GameObject.h>
#include <MeshRenderer.h>
#include "physics/ICollisionReceiver.h"
#include <Scene.h>
#include <Resources.h>

class EnemySword : public GameObject, public Physics::ICollisionReceiver {
public:
    EnemySword();

    void Update();
    void OnCollisionEnter(SceneNode* other) override;
    void OnCollisionExit(SceneNode* other) override {};
};
