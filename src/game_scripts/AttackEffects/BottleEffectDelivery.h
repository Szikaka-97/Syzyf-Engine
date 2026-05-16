#pragma once

#include <GameObject.h>
#include <physics/ICollisionReceiver.h>
#include <Scene.h>
#include <functional>
#include "game_scripts/AttackEffects/EffectBase.h"

class BottleEffectDelivery : public GameObject,
                             public Physics::ICollisionReceiver {
public:
    void SetEffectFactory(std::function<void(SceneNode*)> factory) {
        m_Factory = std::move(factory);
    }

    // Visual node śledzony osobno — NIE jako dziecko butelki
    void SetVisualNode(SceneNode* visual) { m_Visual = visual; }

    void Update() {
        // Ręczna synchronizacja pozycji zamiast SetParent
        SceneNode* self = GetNode();
        if (!m_Visual || !self || m_Triggered) return;
        m_Visual->GlobalTransform().Position() = self->GlobalTransform().Position().Value();
        m_Visual->GlobalTransform().Rotation() = self->GlobalTransform().Rotation().Value();
    }

    void OnCollisionEnter(SceneNode* other) override {
        if (!m_Factory || m_Triggered) return;
        m_Triggered = true;

        SceneNode* self = GetNode();
        glm::vec3 hitPos = self
                           ? glm::vec3(self->GlobalTransform().Position())
                           : glm::vec3(0.0f);

        // Usuń visual bezpośrednio — nie jest dzieckiem, więc brak kaskady
        if (m_Visual) {
            GetScene()->QueueDelete(m_Visual);
            m_Visual = nullptr;
        }

        // Utwórz węzeł efektu
        SceneNode* fxNode = GetScene()->CreateNode("EffectNode");
        fxNode->GlobalTransform().Position() = hitPos;
        m_Factory(fxNode);

        if (auto* effect = fxNode->GetObject<EffectBase>())
            effect->Init();

        // Usuń butelkę — nie ma dzieci, brak kaskady → brak crashu
        if (self) GetScene()->QueueDelete(self);
    }

    void OnCollisionExit(SceneNode*) override {}

private:
    std::function<void(SceneNode*)> m_Factory;
    SceneNode* m_Visual    = nullptr;
    bool       m_Triggered = false;
};