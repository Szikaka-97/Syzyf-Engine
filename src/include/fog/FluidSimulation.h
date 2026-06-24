#pragma once

#include "Debug.h"
#include "GameObject.h"

#include "Texture.h"
#include "Shader.h"

class FluidSimulation : public GameObject, public ImGuiDrawable {
public:
    // Velocity texture resolution
    int resolution = 512;
    // How fast the velocity texture returns to normal after being 'drawn' on by the player, higher values -> lower damping
    float damping = 0.98f;
    // Radius of the circle the player affects, and draws with on the velocity texture
    float playerRadius = 0.05f;
    float interactionStrength = 1.0f;
private:
    Texture2D* textureRead = nullptr;
    Texture2D* textureWrite = nullptr;
    ComputeShaderProgram* computeProgram = nullptr;

    glm::vec3 lastPlayerPosition = glm::vec3(0.0f);
public:
    FluidSimulation();

    void Update();
    // Used to retrieve the velocity map which then can get assigned to the fog volume object
    Texture2D* GetVelocityMap();

    void DrawImGui(); 
};
