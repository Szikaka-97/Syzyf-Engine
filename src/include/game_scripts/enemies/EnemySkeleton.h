#pragma once

#include "game_scripts/enemies/AiSimplified.h"
#include <Scene.h>
#include <game_scripts/enemies/EnemyBase.h>

#include <glm/glm.hpp>

class EnemySkeleton : public EnemyBase {
 public:
  void Update();
};
