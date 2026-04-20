#include "scatter/Spawner.h"
#include "Layer.h"
#include "Graphics.h"
#include "fastgltf/core.hpp"
#include "physics/System.h"
#include "scatter/filters/ArrayFilter.h"
#include "scatter/filters/IFilters.h"

#include <glm/ext/matrix_transform.hpp>
#include <imgui.h>
#include <glm/gtc/random.hpp>
#include <Jolt/Jolt.h>
#include <Jolt/Math/Real.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>

namespace Scatter {

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

    if (this->settings.projectionSettings.has_value()) {
        if (auto* physicsSystem = GetScene()->GetComponent<Physics::System>()) {
            joltSystem = physicsSystem->GetJoltSystem();
            scatterTransform = this->GlobalTransform().Value();
            inverseScatterTransform = glm::inverse(scatterTransform);
        } else {
            spdlog::error("Scatter::Spawner::Generate: Tried applying a projection modifier without a physics system, disabling the modifier");
        }
    }

    this->generationFuture = std::async(std::launch::async, [settingsCopy, joltSystem, scatterTransform, inverseScatterTransform]() {
        PointStream currentPoints;
        currentPoints.reserve(settingsCopy.instanceCount * 2);
        glm::vec3 min = -settingsCopy.areaExtents;
        glm::vec3 max = settingsCopy.areaExtents;

        for (int i = 0; i < settingsCopy.instanceCount * 2; i++) {
            currentPoints.push_back(glm::linearRand(min, max));
        }

        std::vector<std::unique_ptr<IPointFilter>> pointPipeline;

        if (settingsCopy.projectionSettings.has_value() && joltSystem) {
            pointPipeline.push_back(std::make_unique<ProjectionFilter>(settingsCopy.projectionSettings.value(), joltSystem, scatterTransform, inverseScatterTransform)); 
        }
        if (settingsCopy.relaxSettings.has_value()) {
            pointPipeline.push_back(std::make_unique<RelaxFilter>(settingsCopy.relaxSettings.value()));
        }

        for (const auto& filter : pointPipeline) {
            currentPoints = filter->Process(currentPoints);
        }

        if (currentPoints.size() > settingsCopy.instanceCount) {
            currentPoints.resize(settingsCopy.instanceCount);
        }

        InstanceStream instances = Spawner::PointsToInstance(currentPoints, settingsCopy);

        std::vector<std::unique_ptr<IInstanceFilter>> instancePipeline;
        if (settingsCopy.arraySettings.has_value()) {
            instancePipeline.push_back(std::make_unique<ArrayFilter>(settingsCopy.arraySettings.value()));
        }
        for (const auto& filter : instancePipeline) {
            instances = filter->Process(instances);
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

InstanceStream Spawner::PointsToInstance(const PointStream& input, Settings settings) {
    InstanceStream output;
    output.reserve(input.size());

    for (const glm::vec3& position : input) {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position);
        glm::vec3 randomRotation = glm::linearRand(settings.minRotation, settings.maxRotation);

        transform = glm::rotate(transform, randomRotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        transform = glm::rotate(transform, randomRotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        transform = glm::rotate(transform, randomRotation.z, glm::vec3(0.0f, 0.0f, 1.0f));

        float randomScale = glm::linearRand(settings.minScale, settings.maxScale);
        transform = glm::scale(transform, glm::vec3(randomScale));

        output.push_back({ transform });
    }
    return output;
}

void Spawner::DrawImGui() {
    // add missing stuff
    //  also add something similar to the particle spawner
    ImGui::InputInt("Instance Count", &this->settings.instanceCount);
    ImGui::InputFloat3("Area Extents", &this->settings.areaExtents.x);

    ImGui::InputFloat("Min Scale", &this->settings.minScale);
    ImGui::InputFloat("Max Scale", &this->settings.maxScale);
    ImGui::InputFloat3("Min Rotation", &this->settings.minRotation.x);
    ImGui::InputFloat3("Max Rotation", &this->settings.maxRotation.x);

    ImGui::Separator();

    bool relaxEnabled = this->settings.relaxSettings.has_value();
    if (ImGui::Checkbox("Relax Positions", &relaxEnabled)) {
        if (relaxEnabled) {
            this->settings.relaxSettings.emplace();
        } else {
            this->settings.relaxSettings.reset();
        }
    }
    if (relaxEnabled) {
        ImGui::InputFloat("Min Distance", &this->settings.relaxSettings.value().minDistance);
        ImGui::InputInt("Max Attempts", &this->settings.relaxSettings.value().maxAttempts);
    }

    ImGui::Separator();

    bool projectionEnabled = this->settings.projectionSettings.has_value();
    if (ImGui::Checkbox("Project On Colliders", &projectionEnabled)) {
        if (projectionEnabled) {
            this->settings.projectionSettings.emplace();
        } else {
            this->settings.projectionSettings.reset();
        }
    }
    if (projectionEnabled) {
        ImGui::InputFloat3("Ray Direction", &this->settings.projectionSettings.value().raycastDirection.x);
        ImGui::InputFloat("Ray Length", &this->settings.projectionSettings.value().raycastLength);
        ImGui::InputFloat("Ray Offset", &this->settings.projectionSettings.value().raycastOffset);
    }

    ImGui::Separator();
    bool arrayEnabled = this->settings.arraySettings.has_value();
    if (ImGui::Checkbox("Array", &arrayEnabled)) {
        if (arrayEnabled) {
            this->settings.arraySettings.emplace();
        } else {
            this->settings.arraySettings.reset();
        }
    }
    if (arrayEnabled) {
        ImGui::InputInt("Array Size", &this->settings.arraySettings.value().arraySize);
        ImGui::InputFloat3("Array Offset", &this->settings.arraySettings.value().arrayOffset.x);
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
