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

    for (int i = 0; i < settings.instanceCount; i++) {
        glm::vec3 randomPosition = glm::linearRand(min, max);

        randomPosition.y = settings.baseLevelY;

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), randomPosition);

        if (settings.randomRotationY) {
            float randomRotation = glm::linearRand(0.0f, glm::two_pi<float>());
            transform = glm::rotate(transform, randomRotation, glm::vec3(0.0f, 1.0f, 0.0f));
        }

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
        this->settings.instanceCount,
        BoundingBox::CenterAndExtents(glm::vec3(0.0f), this->settings.areaExtents),
        Layer::Default
    );
}

void Scatter::DrawImGui() {
    // add missing stuff
    //  also add something similar to the particle spawner
    // also make it so the instance count doesn't change until you press generate
    ImGui::InputInt("Instance Count", &this->settings.instanceCount);
    ImGui::InputFloat3("Area Extents", &this->settings.areaExtents.x);
    ImGui::InputFloat("Min Scale", &this->settings.minScale);
    ImGui::InputFloat("Max Scale", &this->settings.maxScale);

    if (ImGui::Button("Generate")) {
        this->Generate();
    }
}
