#pragma once
#include "ComboEffectBase.h"
#include "game_scripts/FireParticles.h"
#include <glm/gtc/constants.hpp>
#include <random>
#include <vector>

class ComboExplodeFire : public ComboEffectBase {
public:
  float burnDamagePerTick = 5.0f;
  float burnInterval      = 1.0f;
  int   debrisCount       = 7;
  float debrisMinRange    = 3.0f;
  float debrisMaxRange    = 15.0f;

  void Update();

private:
  struct FireDebris {
    SceneNode* node     = nullptr;
    glm::vec3  velocity = {};
    bool       landed   = false;
  };

  bool                    m_Initialized = false;
  std::vector<FireDebris> m_Debris;
  std::mt19937            m_Rng{ std::random_device{}() };

  void ApplyTo(EnemyBase* enemy);
  void SpawnDebris();
  void UpdateDebris(float dt);
  void CleanupDebris();
protected:

  void  OnInit()          override;
};