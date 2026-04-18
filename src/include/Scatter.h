#pragma once

#include "Debug.h"
#include "GameObject.h"
#include "Mesh.h"
#include "Material.h"

#include <glm/glm.hpp>

struct ScatterInstanceData {
    glm::mat4 transform;
};

struct ScatterSettings {
    int instanceCount = 1000;

    glm::vec3 areaExtents = glm::vec3(50.0f, 0.0f, 50.0f);

    float baseLevelY = 0.0f; // change to somethign else later perhaps

    float minScale = 1.0f;
    float maxScale = 1.0f;

    bool randomRotationY = true;
};

class Scatter : public GameObject, public ImGuiDrawable {
private:
    Mesh* mesh = nullptr;
    std::unique_ptr<Material> material = nullptr;
    ScatterSettings settings;

    std::vector<ScatterInstanceData> instanceData;
    GLuint instanceBuffer = 0;
public:
    Scatter(Mesh* mesh, std::unique_ptr<Material> material, ScatterSettings settings = {});
    ~Scatter();

    void Generate();
    void Render();

    void DrawImGui();
};
