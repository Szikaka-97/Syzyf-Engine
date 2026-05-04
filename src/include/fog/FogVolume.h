#pragma once

#include "Texture.h"
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
    float k = 0.005f;
    float transmittanceThreshold = 0.001f;

    float bias = 0.005f;
    unsigned int maxSteps = 64;

    glm::vec3 scatteringColor = glm::vec3(1.0f);
    float emissiveStrength = 0.0f;
    Texture2D* colorRamp = nullptr;
    
    Texture3D* noiseTexture = nullptr;
    float noiseScale = 0.01f;
    glm::vec3 windDirection = { 0.0f, 0.0f, 0.0f };
    
    float coverage = 0.45f;
    float sharpness = 2.5;
private:
    Mesh *mesh;
    Material *material;
public:
    FogVolume();

    virtual void Render();
    virtual void DrawImGui();
};
