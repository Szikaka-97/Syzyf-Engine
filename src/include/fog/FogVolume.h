#pragma once

#include <Debug.h>
#include <GameObject.h>
#include <glm/glm.hpp>
#include <glad/glad.h>

class Mesh;
class Material;

class FogVolume : public GameObject, public ImGuiDrawable {
public:
  float stepSize = 0.1f;
  float scatteringDensity = 0.7f;
  float absorptionDensity = 0.03f;
  glm::vec3 scatteringColor = glm::vec3(1.0f);
  float k = 0.005f;
  float transmittanceThreshold = 0.001f;

  float bias = 0.005f;
  unsigned int maxSteps = 64;
private:
  Mesh *mesh;
  Material *material;
public:
  FogVolume();

  virtual void Render();
  virtual void DrawImGui();
};
