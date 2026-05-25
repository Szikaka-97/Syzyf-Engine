#pragma once

#include <GameObject.h> // Ensure this include is correct and resolves the GameObject definition
#include "./include/game_scripts/enemies/EnemyBase.h"
#include <MeshRenderer.h>
#include "GltfImporter.h"
#include "physics/Body.h"
#include "physics/ICollisionReceiver.h"
#include "game_scripts/AttackEffects/EffectsManager.h"
#include <Scene.h>
#include <Resources.h>

class EnemySword : public GameObject, public Physics::ICollisionReceiver {
public:
   EnemySword() {
       SceneNode* enemyModel = GltfImporter::LoadScene(
           GetScene(), "./res/models/sword.glb", "sword");
       enemyModel->SetParent(GetNode());
   }
   void Update();
   void OnCollisionEnter(SceneNode* other) override;
   void OnCollisionExit(SceneNode* other) override {}   ;
};
