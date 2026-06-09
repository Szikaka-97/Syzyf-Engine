#include "ParticleSpawner.h"

#include "Layer.h"
#include "Material.h"
#include "TimeSystem.h"
#include "Graphics.h"

#include "imgui.h"
#include <glm/gtc/random.hpp>

ParticleSpawner::ParticleSpawner(Mesh* mesh, Material* material, ParticleSpawnerSettings settings) : mesh(mesh), material(material), settings(settings) { }

void ParticleSpawner::Awake() {
    this->ditherTexture = this->GetScene()->Resources()->Get<Texture2D>(DITHER_TEXTURE_PATH, Texture::TechnicalMapXYZ);
    ComputeShaderProgram* shader = new ComputeShaderProgram(COMPUTE_SHADER_PATH);
    this->computeDispatch.reset(new ComputeShaderDispatch(shader));

    this->initialParticleData.reserve(settings.maxParticles);

    glm::vec3 spawnExtents = settings.wrapAround ? settings.areaExtents : settings.emissionShapeExtents;
    glm::vec3 min = -spawnExtents;
    glm::vec3 max = spawnExtents;

    for (int i = 0; i < settings.maxParticles; i++) {
        ParticleData p;

        glm::vec3 randomPosition = glm::linearRand(min, max);
        glm::vec3 randomVelocity = glm::linearRand(settings.minVelocity, settings.maxVelocity);

        float randomLifetime = glm::linearRand(settings.minLifetime, settings.maxLifetime);

        if (settings.continuous) {
            float spawnDelay = (static_cast<float>(i) / static_cast<float>(settings.maxParticles)) * settings.maxLifetime;
            p.lifetime.x = -spawnDelay;
            glm::vec3 randomPosition = glm::linearRand(min, max);
            p.position = glm::vec4(randomPosition, glm::linearRand(settings.minScale, settings.maxScale));
        } else if (settings.wrapAround) {
            p.lifetime.x = glm::linearRand(0.0f, randomLifetime);
            glm::vec3 randomPosition = glm::linearRand(min, max);
            p.position = glm::vec4(randomPosition, glm::linearRand(settings.minScale, settings.maxScale));
        } else {
            float spawnDelay = (static_cast<float>(i) / static_cast<float>(settings.maxParticles)) * settings.maxLifetime;
            p.lifetime.x = -spawnDelay;
            p.position = glm::vec4(0.0f, -99999999.0f, 0.0f, glm::linearRand(settings.minScale, settings.maxScale));
        }

        float randomAngle = glm::linearRand(settings.minInitialAngle, settings.maxInitialAngle);
        float randomAngularVelocity = glm::linearRand(settings.minAngularVelocity, settings.maxAngularVelocity);

        p.velocity = glm::vec4(randomVelocity, 1.0f);
        p.lifetime.y = randomLifetime;
        p.lifetime.z = randomAngle;
        p.lifetime.w = randomAngularVelocity;

        this->initialParticleData.push_back(p);
    }

    glGenBuffers(1, &particleBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, settings.maxParticles * sizeof(ParticleData), this->initialParticleData.data(), GL_DYNAMIC_COPY);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    this->computeDispatch->GetData()->BindStorageBuffer("ParticleBuffer", this->particleBuffer);
}

ParticleSpawner::~ParticleSpawner() {
    if (this->particleBuffer != 0) {
        glDeleteBuffers(1, &this->particleBuffer);
    }
}

void ParticleSpawner::Update() {
    if (this->material == nullptr) {
        return;
    }

    // rename so either nothing has the 'u' prefix or every uniform has it
    this->material->SetValue("areaCenter", this->GlobalTransform().Position().value);

    // change later
    this->material->SetValue("billboardMode", static_cast<unsigned int>(this->settings.billboardMode));

    this->material->SetValue("alphaMode", static_cast<unsigned int>(this->settings.alphaMode));
    if (this->settings.alphaMode == AlphaMode::Dither) {
        this->material->SetValue("ditherTex", this->ditherTexture);
    }

    if (this->settings.alphaMode != AlphaMode::Disabled) {
        this->material->SetValue("proximityFadeMode", static_cast<unsigned int>(this->settings.enableProximityFade ? 1 : 0));
        if (this->settings.enableProximityFade) {
            this->material->SetValue("proximityFadeMin", this->settings.proximityFadeMin);
            this->material->SetValue("proximityFadeMax", this->settings.proximityFadeMax);
        }

        this->material->SetValue("distanceFadeMode", static_cast<unsigned int>(this->settings.enableDistanceFade ? 1 : 0));
        if (this->settings.enableDistanceFade) {
            this->material->SetValue("distanceFadeMin", this->settings.distanceFadeMin);
            this->material->SetValue("distanceFadeMax", this->settings.distanceFadeMax);
        }

        this->material->SetValue("lifetimeFadeMode", static_cast<unsigned int>(this->settings.enableLifetimeFade ? 1 : 0));
        if (this->settings.enableLifetimeFade) {
            this->material->SetValue("lifetimeFadeIn", this->settings.lifetimeFadeIn);
            this->material->SetValue("lifetimeFadeOut", this->settings.lifetimeFadeOut);
        }

        this->material->SetValue("enableDepthFade", static_cast<unsigned int>(this->settings.enableDepthFade ? 1 : 0)); // XD
        if (this->settings.enableDepthFade == true) {
            this->material->SetValue("depthTex", static_cast<Texture2D*>(this->GetScene()->GetGraphics()->GetMainFramebuffer()->GetDepthTexture()));
            this->material->SetValue("depthFadeDistance", this->settings.depthFadeDistance);
        }
    }

    this->material->SetValue("useColorRamp", static_cast<unsigned int>(this->settings.useColorRamp ? 1 : 0));

    this->material->SetValue("rotateY", static_cast<unsigned int>(this->settings.rotateY ? 1 : 0));

    glm::vec3 center = this->GlobalTransform().Position();
    glm::vec3 extents = this->settings.areaExtents;

    ComputeDispatchData* computeDispatchData = this->computeDispatch->GetData();
    computeDispatchData->BindStorageBuffer("ParticleBuffer", this->particleBuffer);
    
    computeDispatchData->SetValue("uAreaCenter", center);
    computeDispatchData->SetValue("uEmissionShapeExtents", this->settings.emissionShapeExtents);
    computeDispatchData->SetValue("uAreaExtents", this->settings.areaExtents);
    computeDispatchData->SetValue("uDeltaTime", Time::Delta());
    computeDispatchData->SetValue("uWrapAround", static_cast<unsigned int>(this->settings.wrapAround));
    computeDispatchData->SetValue("uEnableLifetime", static_cast<unsigned int>(this->settings.enableLifetime));

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, this->particleBuffer);

    GLuint workGroups = (this->settings.maxParticles + 63) / 64;
    this->computeDispatch->Dispatch(workGroups, 1, 1);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void ParticleSpawner::Render() {
    if (!this->mesh || !this->material) {
        spdlog::error("ParticleSpawner: [{}] mesh or material missing", GetNode()->GetName());
        return;
    }

    this->GetScene()->GetGraphics()->DrawMeshInstanced(
        this->mesh,
        0,
        this->material,
        this->GlobalTransform().Value(),
        this->settings.maxParticles,
        BoundingBox::CenterAndExtents(glm::vec3(0.0f), this->settings.areaExtents),
        this->particleBuffer,
        Layer::Default
    );
}

// Move set value to here perhaps?
void ParticleSpawner::DrawImGui() {
    ImGui::Text("Mesh:");
    ImGui::SameLine(100.0f);
    std::string meshLabel = this->mesh ? "Mesh Loaded" : "Missing Mesh";
    ImGui::Button(meshLabel.c_str(), ImVec2(-1, 0));

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_FILE_PATH")) {
            const char* droppedFilePath = static_cast<const char*>(payload->Data);
            std::filesystem::path path(droppedFilePath);

            std::string extension = path.extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

            if (extension == ".obj") {
                this->mesh = this->GetScene()->Resources()->Get<Mesh>(path.string());
            } else {
                spdlog::warn("ParticleSpawner: Invalid file type dropped. Expected '.obj'");
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::Text("Material:");
    ImGui::SameLine(100.0f);
    std::string materialLabel = this->mesh ? "Material Loaded" : "Missing Material";
    ImGui::Button(materialLabel.c_str(), ImVec2(-1, 0));

    ImGui::Separator();

    if (this->mesh == nullptr || this->material == nullptr) {
        ImGui::BeginDisabled();
    }

    if (this->material != nullptr) {
        if (ImGui::CollapsingHeader("Material")) {
            ImGui::Spacing();
            Debug::Property(*this->material, "Material Properties");
            ImGui::Spacing();
            ImGui::Separator();
        }
    }

    const char* billboardModes[] = { "Disabled", "Enabled", "Z" };
    int currentBillboardMode = static_cast<int>(this->settings.billboardMode);
    if (ImGui::Combo("Billboard Mode", &currentBillboardMode, billboardModes, IM_ARRAYSIZE(billboardModes))) {
        this->settings.billboardMode = static_cast<BillboardMode>(currentBillboardMode);
    }

    if (this->settings.alphaMode != AlphaMode::Disabled) {
        if (this->settings.enableProximityFade) {
            ImGui::InputFloat("Proximity Fade Min", &this->settings.proximityFadeMin);
            ImGui::InputFloat("Proximity Fade Max", &this->settings.proximityFadeMax);
        }

        if (this->settings.enableDistanceFade) {
            ImGui::InputFloat("Distance Fade Min", &this->settings.distanceFadeMin);
            ImGui::InputFloat("Distance Fade Max", &this->settings.distanceFadeMax);
        }

        if (this->settings.enableLifetimeFade) {
            ImGui::InputFloat2("Lifetime Fade Min", &this->settings.lifetimeFadeIn[0]);
            ImGui::InputFloat2("Lifetime Fade Max", &this->settings.lifetimeFadeOut[0]);
        }

        ImGui::Checkbox("Enable Depth Fade", &this->settings.enableDepthFade);
        if (this->settings.enableDepthFade == true) {
            ImGui::InputFloat("Depth Fade Distance", &this->settings.depthFadeDistance);
        }

        ImGui::Checkbox("Use Color Ramp", &this->settings.useColorRamp);
    }

    ImGui::Checkbox("Rotate Y", &this->settings.rotateY);
    if (this->mesh == nullptr || this->material == nullptr) {
        ImGui::EndDisabled();
    }
}
