#pragma once

#include <GameObject.h>
#include <physics/ICollisionReceiver.h>
#include <Scene.h>

#include "game_scripts/AttackEffects/EffectBase.h"

class BottleEffectDelivery : public GameObject,
                              public Physics::ICollisionReceiver {
public:
    void SetEffect(EffectBase* effect) { m_Effect = effect; }

    /*void OnCollisionEnter(SceneNode* other) override {
        if (!m_Effect || m_Triggered) return;
        m_Triggered = true;

        glm::vec3 hitPos = myNode
                           ? glm::vec3(myNode->GlobalTransform().Position())
                           : glm::vec3(0.0f);

        SceneNode* fxNode = m_Effect->GetNode();
        if (fxNode) {
            fxNode->GlobalTransform().Position() = hitPos;
            fxNode->SetEnabled(true);
        }

        m_Effect->Init();

        if (myNode) GetScene()->QueueDelete(myNode);
    }*/
void OnCollisionEnter(SceneNode* other) override {  
   if (!m_Effect || m_Triggered) return;  
   m_Triggered = true;  

   glm::vec3 hitPos = myNode  
                      ? glm::vec3(myNode->GlobalTransform().Position())  
                      : glm::vec3(0.0f);  

   // Clone() creates a new instance of the effect with its own node  
   EffectBase* cloned = m_Effect->Clone();  
   if (!cloned) { spdlog::error("BottleEffectDelivery: Clone() returned nullptr"); return; }  

   SceneNode* fxNode = GetScene()->CreateNode("EffectNode");  
   fxNode->GlobalTransform().Position() = hitPos;  

   // Use AddObject<EffectBase> with the cloned object  
   fxNode->AddObject<EffectBase>(cloned);  

   cloned->Init();  
   fxNode->SetEnabled(true);  

   if (myNode) GetScene()->QueueDelete(myNode);  
}


    void OnCollisionExit(SceneNode*) override {}

private:
    EffectBase* m_Effect    = nullptr;
    SceneNode*   myNode     = GetNode();
    bool        m_Triggered = false;
};