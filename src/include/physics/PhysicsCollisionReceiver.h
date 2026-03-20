#pragma once
class SceneNode;

class IPhysicsCollisionReceiver {
public:
  virtual ~IPhysicsCollisionReceiver() = default;
  virtual void OnCollisionEnter(SceneNode* otherNode) = 0;
};
