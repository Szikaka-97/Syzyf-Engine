#pragma once

#include "Debug.h"
#include "GameObject.h"
#include "Mesh.h"
#include "Material.h"
#include "scatter/modifiers/ArrayModifier.h"
#include "scatter/modifiers/IModifiers.h"
#include "scatter/modifiers/ProjectionModifier.h"
#include "scatter/modifiers/RelaxModifier.h"
#include "scatter/modifiers/TransformModifier.h"

#include <future>

#include <glm/glm.hpp>

namespace Scatter {

struct InstanceData {
    glm::mat4 transform;
};

struct DrawElementsIndirectCommand {
    GLuint count;
    GLuint instanceCount;
    GLuint firstIndex;
    GLuint baseVertex;
    GLuint baseInstance;
};

using ModifierSettings = std::variant<ProjectionSettings, RelaxSettings, ArraySettings, TransformSettings>;

struct Settings {
    int instanceCount = 1000;
    glm::vec3 areaExtents = glm::vec3(50.0f, 0.0f, 50.0f);

    float minScale = 1.0f;
    float maxScale = 1.0f;
    glm::vec3 minRotation = { 0.0f, 0.0f, 0.0f };
    glm::vec3 maxRotation = { 0.0f, 0.0f, 0.0f };

    std::vector<ModifierSettings> modifiers;
};

class SettingsBuilder {
private:
    Settings settings;

public:
    SettingsBuilder() = default;

    SettingsBuilder& WithInstanceCount(int count);
    SettingsBuilder& WithAreaExtents(glm::vec3 extents);

    SettingsBuilder& AddProjection(const ProjectionSettings& config);
    SettingsBuilder& AddRelax(const RelaxSettings& config);
    SettingsBuilder& AddTransform(const TransformSettings& config);
    SettingsBuilder& AddArray(const ArraySettings& config);
    SettingsBuilder& AddModifier(const ModifierSettings& modifier);

    Settings Build();
};

class Spawner : public GameObject, public ImGuiDrawable {
private:
    Mesh* mesh = nullptr;
    Material* material = nullptr;
    Settings settings;

    std::vector<InstanceData> instanceData;
    GLuint instanceBuffer = 0;

    std::future<std::vector<InstanceData>> generationFuture;
    std::atomic<bool> isGenerating{false};

    GLuint culledInstanceBuffer = 0;
    GLuint indirectBuffer =0;

    std::unique_ptr<ComputeShaderDispatch> cullDispatch;
public:
    Spawner(Mesh* mesh, Material* material, Settings settings = {});
    ~Spawner();

    void Generate();
    
    void Update();
    void Render();

    void DrawImGui();
private:
    void UploadToGPU();
    void InitCulling();
    const char* GetModifierName(const ModifierSettings& modifier);
};
}
