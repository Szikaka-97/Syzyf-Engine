#pragma once

#include "Debug.h"
#include "GameObject.h"
#include "Mesh.h"
#include "Material.h"
#include "scatter/filters/ArrayFilter.h"
#include "scatter/filters/ProjectionFilter.h"
#include "scatter/filters/RelaxFilter.h"

#include <future>

#include <glm/glm.hpp>

namespace Scatter {

struct InstanceData {
    glm::mat4 transform;
};

struct Settings {
    int instanceCount = 1000;

    glm::vec3 areaExtents = glm::vec3(50.0f, 0.0f, 50.0f);

    float minScale = 1.0f;
    float maxScale = 1.0f;

    glm::vec3 minRotation = { 0.0f, 0.0f, 0.0f };
    glm::vec3 maxRotation = { 0.0f, 0.0f, 0.0f };

    std::optional<ProjectionSettings> projectionSettings;
    std::optional<RelaxSettings> relaxSettings;
    std::optional<ArraySettings> arraySettings;
};

class Spawner : public GameObject, public ImGuiDrawable {
private:
    Mesh* mesh = nullptr;
    std::unique_ptr<Material> material = nullptr;
    Settings settings;

    std::vector<InstanceData> instanceData;
    GLuint instanceBuffer = 0;

    std::future<std::vector<InstanceData>> generationFuture;
    std::atomic<bool> isGenerating{false};
public:
    Spawner(Mesh* mesh, std::unique_ptr<Material> material, Settings settings = {});
    ~Spawner();

    void Generate();
    
    void Update();
    void Render();

    void DrawImGui();
private:
    void UploadToGPU();
    static InstanceStream PointsToInstance(const PointStream& input, Settings settings);
};
}
