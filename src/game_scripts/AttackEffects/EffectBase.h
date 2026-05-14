#pragma once

#include <GameObject.h>
#include <Scene.h>
#include <Material.h>
#include <TimeSystem.h>
#include <glm/glm.hpp>
#include <vector>

#include "enemies/EnemyBase.h"

class EffectBase : public GameObject {
public:
    float radius          = 1.0f;
    float speed           = 5.0f;   
    int   modifier        = 2;
    int   ingredientCount = 1;
    bool  special1        = false;
    bool  special2        = false;

    void SetEffectMaterial(Material* mat) { m_EffectMaterial = mat; }

    void Init();

    void Update();

protected:

    virtual void OnApplySpecials() {}

    virtual void OnApplyToEnemy(EnemyBase* enemy) = 0;

    virtual void OnUpdate() {}

    virtual float GetMaxLifetime() const { return radius; }

    std::vector<EnemyBase*> ScanEnemiesInRadius() const;

    glm::vec3 GetPosition() const;

    float m_Lifetime = 0.0f;

    SceneNode* myNode = nullptr; 

private:
    Material* m_EffectMaterial = nullptr;

    void UpdateShaderVisual();
};