#pragma once

#include <AiSimplified.h>
#include <Player.h>
#include <Scene.h>
#include <enemies/EnemyBase.h>

#include <glm/glm.hpp>

class EnemySkeleton : public EnemyBase {
 public:
  void Update();
};
