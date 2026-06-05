#pragma once

#include <GameObject.h>
#include <Scene.h>
#include <Material.h>
#include <TimeSystem.h>
#include <glm/glm.hpp>
#include <vector>

#include "game_scripts/enemies/EnemyBase.h"
#include <MeshRenderer.h>

class EffectBase : public GameObject {
public:
    float radius          = 1.0f;
    float speed           = 5.0f;
    int   modifier        = 2;
    int   ingredientCount = 1;
    bool  special1        = false;
    bool  special2        = false;

    void SetVisual(Mesh* mesh, Material* mat);
    void SetEffectRenderer(Mesh* mesh, Material* mat);

    void Init();

    void Update();

    EffectBase()          = default;
    virtual ~EffectBase() = default;

protected:
    virtual void  OnInit()                    {}

    virtual void  OnApplySpecials()           {}

    virtual void  OnApplyToEnemy(EnemyBase*)  {}

    virtual void  OnUpdate()                  {}

    virtual float GetMaxLifetime() const      { return radius; }

    std::vector<EnemyBase*> ScanEnemiesInRadius() const;
    glm::vec3               GetPosition()          const;

    float      m_Lifetime = 0.0f;

    SceneNode* myNode     = nullptr;

private:
    void UpdateShaderVisual();
};