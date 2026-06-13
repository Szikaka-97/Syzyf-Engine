#include "Debug.h"
#include "GameObject.h"

#include "Texture.h"
#include "Shader.h"

class FluidSimulation : public GameObject, public ImGuiDrawable {
public:
    int resolution = 512;
    float damping = 0.98f;
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
    Texture2D* GetVelocityMap();

    void DrawImGui(); 
};
