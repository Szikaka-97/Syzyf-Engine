#include "scatter/Spawner.h"
#include "Layer.h"
#include "Graphics.h"
#include "physics/System.h"
#include "scatter/modifiers/ArrayModifier.h"
#include "scatter/modifiers/IModifiers.h"

#include <glm/ext/matrix_transform.hpp>
#include <imgui.h>
#include <glm/gtc/random.hpp>
#include <Jolt/Jolt.h>
#include <Jolt/Math/Real.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>

namespace Scatter {

SettingsBuilder& SettingsBuilder::WithInstanceCount(int count) {
    settings.instanceCount = count;
    return *this;
}

SettingsBuilder& SettingsBuilder::WithAreaExtents(glm::vec3 extents) {
    settings.areaExtents = extents;
    return *this;
}

SettingsBuilder& SettingsBuilder::AddProjection(const ProjectionSettings& config) {
    settings.modifiers.push_back(config);
    return *this;
}

SettingsBuilder& SettingsBuilder::AddRelax(const RelaxSettings& config) {
    settings.modifiers.push_back(config);
    return *this;
}

SettingsBuilder& SettingsBuilder::AddTransform(const TransformSettings& config) {
    settings.modifiers.push_back(config);
    return *this;
}

SettingsBuilder& SettingsBuilder::AddArray(const ArraySettings& config) {
    settings.modifiers.push_back(config);
    return *this;
}

SettingsBuilder& SettingsBuilder::AddModifier(const ModifierSettings& modifier) {
    settings.modifiers.push_back(modifier);
    return *this;
}

Settings SettingsBuilder::Build() {
    return std::move(settings); 
}

Spawner::Spawner(Mesh* mesh, std::unique_ptr<Material> material, Settings settings) : mesh(mesh), material(std::move(material)), settings(settings) {
    Generate();
}

Spawner::~Spawner() {
    if (this->instanceBuffer != 0) {
        glDeleteBuffers(1, &this->instanceBuffer);
    }
}

void Spawner::Generate() {
    if (this->isGenerating) {
        spdlog::warn("Scatter::Spawner::Generate: Already generating, returning");
    }

    this->isGenerating = true;
    Settings settingsCopy = this->settings;

    JPH::PhysicsSystem* joltSystem = nullptr;
    glm::mat4 scatterTransform = glm::mat4(1.0f);
    glm::mat4 inverseScatterTransform = glm::mat4(1.0f);

    if (auto* physicsSystem = GetScene()->GetComponent<Physics::System>()) {
        joltSystem = physicsSystem->GetJoltSystem();
    }

    this->generationFuture = std::async(std::launch::async, [settingsCopy, joltSystem, scatterTransform, inverseScatterTransform]() {
        PointStream currentPoints;
        currentPoints.reserve(settingsCopy.instanceCount * 2);

        glm::vec3 min = -settingsCopy.areaExtents;
        glm::vec3 max = settingsCopy.areaExtents;

        for (int i = 0; i < settingsCopy.instanceCount * 2; i++) {
            currentPoints.push_back(glm::linearRand(min, max));
        }

        InstanceStream instances;
        bool hasTransformed = false;

        for (const auto& modifierSettings : settingsCopy.modifiers) {
            std::visit([&](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;

                // Point Modifier 
                if constexpr (std::is_same_v<T, ProjectionSettings>) {
                    if (!hasTransformed && joltSystem) {
                        ProjectionModifier modifier(arg, joltSystem, scatterTransform, inverseScatterTransform);
                        currentPoints = modifier.Process(currentPoints);
                    }
                } else if constexpr (std::is_same_v<T, RelaxSettings>) {
                    if (!hasTransformed) {
                        RelaxModifier modifier(arg);
                        currentPoints = modifier.Process(currentPoints);
                    }
                }
                // PointToInstance Bridge Modifier 
                else if constexpr (std::is_same_v<T, TransformSettings>) {
                if (!hasTransformed) {
                    if (currentPoints.size() > settingsCopy.instanceCount) {
                        currentPoints.resize(settingsCopy.instanceCount);
                    }

                    TransformModifier modifier(arg);
                    instances = modifier.Process(currentPoints);
                    hasTransformed = true;
                }
                }
                // Instance Modifier 
                else if constexpr (std::is_same_v<T, ArraySettings>) {
                    if (hasTransformed) {
                        ArrayModifier modifier(arg);
                        instances = modifier.Process(instances);
                    }
                }
            }, modifierSettings);
        }

        if (!hasTransformed) {
            if (currentPoints.size() > settingsCopy.instanceCount) {
                currentPoints.resize(settingsCopy.instanceCount);
            }
            TransformSettings defaultSettings;
            TransformModifier modifier(defaultSettings);
            instances = modifier.Process(currentPoints);
        }

        return instances;
    });
}

void Spawner::Update() {
    if (this->isGenerating && generationFuture.valid()
            && generationFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
        this->instanceData = generationFuture.get();

        UploadToGPU();

        isGenerating = false;
    }
}

void Spawner::Render() {
    if (!this->mesh || !this->material) {
        spdlog::error("Scatter: missing mesh or material");
        return;
    }

    this->GetScene()->GetGraphics()->DrawMeshInstanced(
        this->mesh,
        0,
        this->material.get(),
        this->GlobalTransform(),
        this->instanceData.size(),
        BoundingBox::CenterAndExtents(glm::vec3(0.0f), this->settings.areaExtents),
        Layer::Default
    );
}

void Spawner::UploadToGPU() {
    if (this->instanceBuffer == 0) {
        glGenBuffers(1, &this->instanceBuffer);
    }

    std::size_t count = this->instanceData.size();

    if (count > 0) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->instanceBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, count * sizeof(InstanceData), this->instanceData.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        this->material->BindStorageBuffer("ScatterInstanceBuffer", this->instanceBuffer);
    }
}

void Spawner::DrawImGui() {
    // add missing stuff
    //  also add something similar to the particle spawner
    ImGui::InputInt("Instance Count", &this->settings.instanceCount);
    ImGui::InputFloat3("Area Extents", &this->settings.areaExtents.x);

    ImGui::Separator();

    for (auto& modifier : this->settings.modifiers) {
        std::visit([](auto& modifier) {
            modifier.DrawImGui();
            ImGui::Separator();
        }, modifier);
    }

    if (this->isGenerating) {
        ImGui::BeginDisabled();
        ImGui::Button("Generating...");
        ImGui::EndDisabled();
    } else {
        if (ImGui::Button("Generate")) {
            this->Generate();
        }
    }

    ImGui::Text("Instance count: %zu", this->instanceData.size());
}
}
