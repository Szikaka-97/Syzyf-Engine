#pragma once
 
enum class EffectType {
    EXPLODE,
    FIRE,
    PETRIFY,
    TORNADO,
    CONFUSE
};

#include <GameObject.h>
#include <Scene.h>
#include <TimeSystem.h>
#include <glm/glm.hpp>
#include <unordered_set>
 
#include "./include/game_scripts/enemies/EnemyBase.h"

class ComboEffectBase : public GameObject {
public:
    float duration        = 2.0f;
    float effect1Strength = 0.5f;  // [0..1]
    float maxEffect1Range = 5.0f;
    float maxEffect1Damage = 30.0f;
 
    float effect2Strength = 0.5f;
 
    // Must be called once after adding to a node
    void Init(float e1Strength, float e1MaxRange, float e1MaxDamage,
              float dur = 2.0f);
 
    float GetRange()  const { return effect1Strength * maxEffect1Range; }
    float GetDamage() const { return effect1Strength * maxEffect1Damage; }
 
    std::vector<EnemyBase*> ScanNearbyEnemies() const;
 
    glm::vec3 GetFlatPosition() const;
 
    void Update();
 
    bool IsExpired() const { return m_Expired; }

    void SetEffectRenderer(Mesh* mesh, Material* mat);
 
protected:
    float m_Elapsed = 0.0f;
    bool  m_Expired = false;

    SceneNode* myNode = GetNode();
 
    std::unordered_set<EnemyBase*> m_HitEnemies;
};