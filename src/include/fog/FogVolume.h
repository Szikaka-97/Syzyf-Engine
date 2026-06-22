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
    // Lower values -> higher quality, worse performance
    //  Higher values -> better performance, banding
    float stepSize = 0.1f;
    // Makes the fog more opaque, look thicker
    float scatteringDensity = 0.7f;
    // Makes the fog darker
    float absorptionDensity = 0.03f;
    // Anisotropy, changes the direction of light scattering when looking at a light source
    float k = 0.005f;
    // Stops the calculations early if the transmittance falls below this threshold, better performance the higher it is
    float transmittanceThreshold = 0.001f;

    // Prevents shadow artifacts
    float bias = 0.005f;
    // Better performance at lower values,
    //  Needs to be higher the deeper the volume is
    unsigned int maxSteps = 64;

    // Fog color
    glm::vec3 scatteringColor = glm::vec3(1.0f);
    // Multiplies the fog color, might be used to make the fog glow
    float emissiveStrength = 0.0f;
    // Can be used to give the fog a gradient
    Texture2D* colorRamp = nullptr;
   
    // 3D Noise Texture, allows for non uniform fog
    Texture3D* noiseTexture = nullptr;
    float noiseScale = 0.01f;
    // Direction (and speed) in which the noise texture moves
    //  makes the fog appear to be moving
    glm::vec3 windDirection = { 0.0f, 0.0f, 0.0f };

    // Texture created by the fluid simulation
    //  allows for the player/enemies to affect the fog
    Texture2D* velocityTexture = nullptr;
    float velocityStrength = 2.0f;
   
    // This gets subtracted from the 3d noise value
    float coverage = 0.45f;
    // This multiplies the final 3d noise value
    float sharpness = 2.5;
private:
    Mesh *mesh;
    Material *material;
public:
    FogVolume() = default;

    void Awake();

    virtual void Render();
    virtual void DrawImGui();
};
