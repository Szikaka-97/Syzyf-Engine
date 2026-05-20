#pragma once

#include "./include/game_scripts/enemies/AiSimplified.h"
#include <Player.h>
#include <Scene.h>
#include <./include/game_scripts/enemies/EnemyBase.h>

#include <glm/glm.hpp>

class EnemySkeleton : public EnemyBase {
 public:
  void Update();
};
