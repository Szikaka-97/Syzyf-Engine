#pragma once

#include <GameObject.h>
#include <Scene.h>
#include <Material.h>
#include <TimeSystem.h>
#include <glm/glm.hpp>
#include <vector>

#include "enemies/EnemyBase.h"
#include <MeshRenderer.h>

class EffectBase : public GameObject {
public:
    float radius          = 1.0f;
    float speed           = 5.0f;   
    int   modifier        = 2;
    int   ingredientCount = 1;
    bool  special1        = false;
    bool  special2        = false;

    void SetEffectRenderer(Mesh* mesh, Material* mat);

    void Init();

    void Update();

    //virtual EffectBase* Clone() const { return nullptr; }

    EffectBase() = default;

    //EffectBase(const EffectBase& other)
    //: radius(other.radius), speed(other.speed), modifier(other.modifier),
    //  ingredientCount(other.ingredientCount), special1(other.special1), special2(other.special2),
    //  m_Lifetime(0.0f), myNode(nullptr), m_EffectRenderer(nullptr) {}

    virtual ~EffectBase() = default;


protected:

    virtual void OnApplySpecials() {}

    virtual void OnApplyToEnemy(EnemyBase* enemy) {};

    virtual void OnUpdate() {}

    virtual float GetMaxLifetime() const { return radius; }

    std::vector<EnemyBase*> ScanEnemiesInRadius() const;

    glm::vec3 GetPosition() const;

    float m_Lifetime = 0.0f;

    SceneNode* myNode = nullptr; 

private:
    MeshRenderer* m_EffectRenderer = nullptr;

    void UpdateShaderVisual();
};