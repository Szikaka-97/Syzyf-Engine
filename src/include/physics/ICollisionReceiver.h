#pragma once
class SceneNode;

namespace Physics {
class ICollisionReceiver {
public:
  virtual ~ICollisionReceiver() = default;
  virtual void OnCollisionEnter(SceneNode* otherNode) = 0;
};
}
