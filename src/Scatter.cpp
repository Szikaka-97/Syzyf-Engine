#include "Scatter.h"
#include "Layer.h"
#include "Graphics.h"

#include <glm/ext/matrix_transform.hpp>
#include <imgui.h>
#include <glm/gtc/random.hpp>

Scatter::Scatter(Mesh* mesh, std::unique_ptr<Material> material, ScatterSettings settings) : mesh(mesh), material(std::move(material)), settings(settings) {
    Generate();
}

Scatter::~Scatter() {
    if (this->instanceBuffer != 0) {
        glDeleteBuffers(1, &this->instanceBuffer);
    }
}

void Scatter::Generate() {
    if (this->isGenerating) {
        spdlog::warn("Scatter::Generate: Already generating, returning");
    }

    this->isGenerating = true;
    ScatterSettings settingsCopy = this->settings;

    this->generationFuture = std::async(std::launch::async, [settingsCopy]() {
        std::vector<ScatterInstanceData> newData;
        newData.reserve(settingsCopy.instanceCount * (1 + settingsCopy.arraySize));

        glm::vec3 min = -settingsCopy.areaExtents;
        glm::vec3 max = settingsCopy.areaExtents;

        std::vector<glm::vec3> validPositions;
        if (settingsCopy.relaxSettings.enabled == true) {
            validPositions.reserve(settingsCopy.instanceCount);
        }

        for (int i = 0; i < settingsCopy.instanceCount; i++) {
            glm::vec3 randomPosition;

            if (settingsCopy.relaxSettings.enabled == true) {
                bool positionFound = false;

                for (int attempt = 0; attempt < settingsCopy.relaxSettings.maxAttempts; attempt++) {
                    randomPosition = glm::linearRand(min, max);
                    randomPosition.y = settingsCopy.baseLevelY;
                    
                    bool isOverlapping = false;

                    for (const glm::vec3& existingPosition : validPositions) {
                        if (glm::distance(randomPosition, existingPosition) < settingsCopy.relaxSettings.minDistance) {
                            isOverlapping = true;
                            break;
                        }
                    }

                    if (!isOverlapping) {
                        positionFound = true;
                        break;
                    }
                }

                if (!positionFound) {
                    continue;
                }

                validPositions.push_back(randomPosition);
            } else {
                randomPosition = glm::linearRand(min, max);
                randomPosition.y = settingsCopy.baseLevelY;
            }

            glm::mat4 transform = glm::translate(glm::mat4(1.0f), randomPosition);

            glm::vec3 randomRotation = glm::linearRand(settingsCopy.minRotation, settingsCopy.maxRotation);

            transform = glm::rotate(transform, randomRotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
            transform = glm::rotate(transform, randomRotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
            transform = glm::rotate(transform, randomRotation.z, glm::vec3(0.0f, 0.0f, 1.0f));

            // add an option to make non uniform later
            float randomScale = glm::linearRand(settingsCopy.minScale, settingsCopy.maxScale);
            transform = glm::scale(transform, glm::vec3(randomScale));

            newData.push_back({ transform });

            for (int i = 0; i < settingsCopy.arraySize; i++) {
                newData.push_back({ glm::translate(transform, settingsCopy.arrayOffset * glm::vec3((1 + i)))});
            }
        }

        return newData;
    });
}

void Scatter::Update() {
    if (this->isGenerating && generationFuture.valid()
            && generationFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
        this->instanceData = generationFuture.get();

        UploadToGPU();

        isGenerating = false;
    }
}

void Scatter::Render() {
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

void Scatter::UploadToGPU() {
    if (this->instanceBuffer == 0) {
        glGenBuffers(1, &this->instanceBuffer);
    }

    std::size_t count = this->instanceData.size();

    if (count > 0) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->instanceBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, count * sizeof(ScatterInstanceData), this->instanceData.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        this->material->BindStorageBuffer("ScatterInstanceBuffer", this->instanceBuffer);
    }
}

void Scatter::DrawImGui() {
    // add missing stuff
    //  also add something similar to the particle spawner
    ImGui::InputInt("Instance Count", &this->settings.instanceCount);
    ImGui::InputFloat3("Area Extents", &this->settings.areaExtents.x);

    ImGui::InputFloat("Min Scale", &this->settings.minScale);
    ImGui::InputFloat("Max Scale", &this->settings.maxScale);
    ImGui::InputFloat3("Min Rotation", &this->settings.minRotation.x);
    ImGui::InputFloat3("Max Rotation", &this->settings.maxRotation.x);

    ImGui::InputInt("Array Size", &this->settings.arraySize);
    if (this->settings.arraySize > 0) {
        ImGui::InputFloat3("Array Offset", &this->settings.arrayOffset.x);
    }

    ImGui::Checkbox("Relax Positions", &this->settings.relaxSettings.enabled);
    if (this->settings.relaxSettings.enabled) {
        ImGui::InputFloat("Min Distance", &this->settings.relaxSettings.minDistance);
        ImGui::InputInt("Max Attempts", &this->settings.relaxSettings.maxAttempts);
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
}
