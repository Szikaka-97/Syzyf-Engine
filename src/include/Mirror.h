
#pragma once

#include "GameObject.h"
#include "Viewport.h"

class SceneNode;
class Material;
class Mesh;

class Mirror : public GameObject {
private:
  SceneNode* cameraNode = nullptr;
  Viewport viewport;
  Material* material = nullptr;
  SceneNode* playerNode = nullptr;
public:
  Mirror(Mesh* mesh);

  void Update();
};
