#pragma once

#include "Debug.h"
#include "GameObject.h"
#include "Mesh.h"
#include "Material.h"

#include <future>

#include <glm/glm.hpp>

struct ScatterInstanceData {
    glm::mat4 transform;
};

struct ScatterRelaxSettings {
    bool enabled = false;
    float minDistance = 2.0f;
    // max number of attempts it will try to place the object
    //  before giving up
    int maxAttempts = 30; 
};

struct ScatterSettings {
    int instanceCount = 1000;

    glm::vec3 areaExtents = glm::vec3(50.0f, 0.0f, 50.0f);

    float baseLevelY = 0.0f; // change to somethign else later perhaps

    float minScale = 1.0f;
    float maxScale = 1.0f;

    glm::vec3 minRotation = { 0.0f, 0.0f, 0.0f };
    glm::vec3 maxRotation = { 0.0f, 0.0f, 0.0f };

    int arraySize = 0;
    glm::vec3 arrayOffset = glm::vec3(0.0f, 1.0f, 0.0f);

    // Replace with a 'modifier stack' pattern
    ScatterRelaxSettings relaxSettings = {};
};

class Scatter : public GameObject, public ImGuiDrawable {
private:
    Mesh* mesh = nullptr;
    std::unique_ptr<Material> material = nullptr;
    ScatterSettings settings;

    std::vector<ScatterInstanceData> instanceData;
    GLuint instanceBuffer = 0;

    std::future<std::vector<ScatterInstanceData>> generationFuture;
    std::atomic<bool> isGenerating{false};
public:
    Scatter(Mesh* mesh, std::unique_ptr<Material> material, ScatterSettings settings = {});
    ~Scatter();

    void Generate();
    
    void Update();
    void Render();

    void DrawImGui();
private:
    void UploadToGPU();
};
