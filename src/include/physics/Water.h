#include "physics/ICollisionReceiver.h"
#include "GameObject.h"

#include <unordered_set>

namespace Physics {
class Water : public GameObject, public Physics::ICollisionReceiver {
private:
  std::unordered_set<SceneNode*> submergedNodes;
  float density = 1000.0f;
public:
  void OnCollisionEnter(SceneNode* otherNode) override;
  void OnCollisionExit(SceneNode* otherNode) override;
  void Update();
};
}
