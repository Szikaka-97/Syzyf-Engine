#include "Scatter.h"
#include "Layer.h"
#include "Graphics.h"

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
    this->instanceData.clear();
    this->instanceData.reserve(settings.instanceCount);

    glm::vec3 min = -settings.areaExtents;
    glm::vec3 max = settings.areaExtents;

    std::vector<glm::vec3> validPositions;
    if (this->settings.relaxSettings.enabled == true) {
        validPositions.reserve(settings.instanceCount);
    }

    for (int i = 0; i < settings.instanceCount; i++) {
        glm::vec3 randomPosition;

        if (this->settings.relaxSettings.enabled == true) {
            bool positionFound = false;

            for (int attempt = 0; attempt < this->settings.relaxSettings.maxAttempts; attempt++) {
                randomPosition = glm::linearRand(min, max);
                randomPosition.y = settings.baseLevelY;
                
                bool isOverlapping = false;

                for (const glm::vec3& existingPosition : validPositions) {
                    if (glm::distance(randomPosition, existingPosition) < this->settings.relaxSettings.minDistance) {
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
            randomPosition.y = settings.baseLevelY;
        }

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), randomPosition);

        glm::vec3 randomRotation = glm::linearRand(this->settings.minRotation, this->settings.maxRotation);

        transform = glm::rotate(transform, randomRotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        transform = glm::rotate(transform, randomRotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        transform = glm::rotate(transform, randomRotation.z, glm::vec3(0.0f, 0.0f, 1.0f));

        // add an option to make non uniform late
        float randomScale = glm::linearRand(settings.minScale, settings.maxScale);
        transform = glm::scale(transform, glm::vec3(randomScale));

        this->instanceData.push_back({ transform });
    }

    if (this->instanceBuffer == 0) {
        glGenBuffers(1, &this->instanceBuffer);
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->instanceBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, settings.instanceCount * sizeof(ScatterInstanceData), this->instanceData.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    this->material->BindStorageBuffer("ScatterInstanceBuffer", this->instanceBuffer);
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

void Scatter::DrawImGui() {
    // add missing stuff
    //  also add something similar to the particle spawner
    ImGui::InputInt("Instance Count", &this->settings.instanceCount);
    ImGui::InputFloat3("Area Extents", &this->settings.areaExtents.x);

    ImGui::InputFloat("Min Scale", &this->settings.minScale);
    ImGui::InputFloat("Max Scale", &this->settings.maxScale);
    ImGui::InputFloat3("Min Rotation", &this->settings.minRotation.x);
    ImGui::InputFloat3("Max Rotation", &this->settings.maxRotation.x);

    ImGui::Checkbox("Relax Positions", &this->settings.relaxSettings.enabled);
    if (this->settings.relaxSettings.enabled) {
        ImGui::InputFloat("Min Distance", &this->settings.relaxSettings.minDistance);
        ImGui::InputInt("Max Attempts", &this->settings.relaxSettings.maxAttempts);
    }

    if (ImGui::Button("Generate")) {
        this->Generate();
    }
}
