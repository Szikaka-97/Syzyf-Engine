#include "scatter/Spawner.h"
#include "Frustum.h"
#include "Layer.h"
#include "Graphics.h"
#include "Texture.h"
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

Spawner::Spawner(Mesh* mesh, Material* material, Settings settings) : mesh(mesh), material(material), settings(settings) {
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
        return;
    }

    this->isGenerating = true;
    Settings settingsCopy = this->settings;

    JPH::PhysicsSystem* joltSystem = nullptr;
    glm::mat4 scatterTransform = this->GlobalTransform().Value();
    glm::mat4 inverseScatterTransform = glm::inverse(scatterTransform);

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
    if (!this->mesh || !this->material || this->instanceData.empty()) {
        return;
    }

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, this->indirectBuffer);
    GLuint zero = 0;

    for (int i = 0; i < this->mesh->GetSubMeshCount(); i++) {
        GLuint offsetToInstanceCount = (i * sizeof(DrawElementsIndirectCommand)) + sizeof(GLuint);
        glBufferSubData(GL_DRAW_INDIRECT_BUFFER, offsetToInstanceCount, sizeof(GLuint), &zero);
    }
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

    const auto camera = GetScene()->GetGraphics()->GetMainCamera();
    const Frustum frustum = ComputeFrustum(camera->ViewProjectionMatrix());

    std::array<glm::vec4, 6> planes = {
        glm::vec4(frustum.top.normal, frustum.top.distance),
        glm::vec4(frustum.bottom.normal, frustum.bottom.distance),
        glm::vec4(frustum.left.normal, frustum.left.distance),
        glm::vec4(frustum.right.normal, frustum.right.distance),
        glm::vec4(frustum.nearPlane.normal, frustum.nearPlane.distance),
        glm::vec4(frustum.farPlane.normal, frustum.farPlane.distance)
    };

    ComputeDispatchData* dispatchData = this->cullDispatch->GetData();

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, this->instanceBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, this->culledInstanceBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, this->indirectBuffer);

    dispatchData->SetValue("frustumTop", glm::vec4(frustum.top.normal, frustum.top.distance));
    dispatchData->SetValue("frustumBottom", glm::vec4(frustum.bottom.normal, frustum.bottom.distance));
    dispatchData->SetValue("frustumLeft", glm::vec4(frustum.left.normal, frustum.left.distance));
    dispatchData->SetValue("frustumRight", glm::vec4(frustum.right.normal, frustum.right.distance));
    dispatchData->SetValue("frustumNear", glm::vec4(frustum.nearPlane.normal, frustum.nearPlane.distance));
    dispatchData->SetValue("frustumFar", glm::vec4(frustum.farPlane.normal, frustum.farPlane.distance));

    dispatchData->SetValue("subMeshCount", static_cast<GLuint>(this->mesh->GetSubMeshCount()));
    dispatchData->SetValue("totalInstances", static_cast<GLuint>(this->instanceData.size()));
    dispatchData->SetValue("meshExtents", this->mesh->GetBounds().GetExtents());
    dispatchData->SetValue("meshCenter", this->mesh->GetBounds().center);
    dispatchData->SetValue("spawnerTransform", this->GlobalTransform().Value());

    dispatchData->Bind();

    GLuint workGroups = (this->instanceData.size() + 63) / 64;
    this->cullDispatch->Dispatch(workGroups, 1, 1);

    glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

    // glBindBuffer(GL_DRAW_INDIRECT_BUFFER, this->indirectBuffer);
    // DrawElementsIndirectCommand command;
    // glGetBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0, sizeof(DrawElementsIndirectCommand), &command);
    //
    // for (int i = 1; i < this->mesh->GetSubMeshCount(); i++) {
    //     GLuint offsetToInstanceCount = (i * sizeof(DrawElementsIndirectCommand)) + offsetof(DrawElementsIndirectCommand, instanceCount);
    //     glBufferSubData(GL_DRAW_INDIRECT_BUFFER, offsetToInstanceCount, sizeof(GLuint), &command.instanceCount);
    // }
    // glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

    for (int i = 0; i < this->mesh->GetSubMeshCount(); i++) {
        GLuint byteOffset = i * sizeof(DrawElementsIndirectCommand);

        this->GetScene()->GetGraphics()->DrawMeshIndirect(
            this->mesh,
            i,
            this->material,
            this->GlobalTransform().Value(),
            this->indirectBuffer,
            byteOffset,
            this->culledInstanceBuffer,
            BoundingBox::CenterAndExtents(glm::vec3(0.0f), this->settings.areaExtents + this->mesh->GetBounds().GetExtents() * this->settings.maxScale),
            Layer::Default
        );
    }
}

void Spawner::UploadToGPU() {
    if (this->instanceBuffer == 0) {
        glGenBuffers(1, &this->instanceBuffer);
        InitCulling();
    }

    std::size_t count = this->instanceData.size();

    if (count > 0) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->instanceBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, count * sizeof(InstanceData), this->instanceData.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->culledInstanceBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, count * sizeof(InstanceData), nullptr, GL_DYNAMIC_DRAW);

        std::vector<DrawElementsIndirectCommand> commands(this->mesh->GetSubMeshCount());
        for (int i = 0; i < this->mesh->GetSubMeshCount(); i++) {
            commands[i].count = this->mesh->SubMeshAt(i).GetVertexCount();
            commands[i].instanceCount = 0;
            commands[i].firstIndex = 0;
            commands[i].baseVertex = 0;
            commands[i].baseInstance = 0;
        }

        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, this->indirectBuffer);
        glBufferData(GL_DRAW_INDIRECT_BUFFER, commands.size() * sizeof(DrawElementsIndirectCommand), commands.data(), GL_DYNAMIC_DRAW);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }
}

void Spawner::InitCulling() {
    this->cullDispatch = std::make_unique<ComputeShaderDispatch>("./res/shaders/scatter/scatter_cull.comp");

    glGenBuffers(1, &this->culledInstanceBuffer);
    glGenBuffers(1, &this->indirectBuffer);
}

void Spawner::DrawImGui() {
    const auto ERROR_COLOR = ImVec4(1.0f, 0.0f, 0.2f, 1.0f);

    if (this->isGenerating) {
        ImGui::BeginDisabled();
        ImGui::Button("Generating...");
        ImGui::EndDisabled();
    } else if (this->mesh == nullptr || this->material == nullptr) {
        ImGui::BeginDisabled();
        ImGui::Button("Generate");
        ImGui::EndDisabled();

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("Missing mesh or material.");
        }
    } else {
        if (ImGui::Button("Generate")) {
            this->Generate();
        }
    }

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
                spdlog::warn("Scatter Spawner: Invalid file type dropped. Expected '.obj'");
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::Text("Material:");
    ImGui::SameLine(100.0f);
    std::string materialLabel = this->mesh ? "Material Loaded" : "Missing Material";
    ImGui::Button(materialLabel.c_str(), ImVec2(-1, 0));


    if (this->mesh == nullptr || this->material == nullptr) {
        ImGui::BeginDisabled();
    }

    ImGui::Separator();

    ImGui::Text("Instance count: %zu", this->instanceData.size());

    ImGui::InputInt("Instance Count", &this->settings.instanceCount);
    ImGui::InputFloat3("Area Extents", &this->settings.areaExtents.x);

    ImGui::Separator();

    std::optional<std::size_t> modifierToDelete;
    std::optional<std::pair<std::size_t, std::size_t>> modifierToMove;

    bool transformModifierUsed = false;
    for (std::size_t i = 0; i < this->settings.modifiers.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));

        bool keepOpen = true;
        const auto& currentModifier = this->settings.modifiers[i];

        bool isPointModifier = std::holds_alternative<ProjectionSettings>(currentModifier) || 
                               std::holds_alternative<RelaxSettings>(currentModifier);
        bool isTransformModifier = std::holds_alternative<TransformSettings>(currentModifier);
        bool isInstanceModifier = std::holds_alternative<ArraySettings>(currentModifier);

        if (isTransformModifier) transformModifierUsed = true;
        bool isWrongOrder = false;
        std::string errorMessage = "";
        
        if (isInstanceModifier && !transformModifierUsed) {
            isWrongOrder = true;
            errorMessage = "Warning: Array must be placed AFTER a Transform modifier.";
        } else if (isPointModifier && transformModifierUsed) {
            isWrongOrder = true;
            errorMessage = "Warning: Point modifiers must be placed BEFORE Transform.";
        }

        std::string header = GetModifierName(currentModifier);
        if (isWrongOrder) {
            ImGui::PushStyleColor(ImGuiCol_Text, ERROR_COLOR);
        }

        bool isOpen = ImGui::CollapsingHeader(header.c_str(), &keepOpen, ImGuiTreeNodeFlags_DefaultOpen);
        if (ImGui::IsItemHovered() && isWrongOrder) {
            ImGui::BeginTooltip();
            ImGui::TextColored(ERROR_COLOR, "%s", errorMessage.c_str());
            ImGui::EndTooltip();
        }

        if (isWrongOrder) {
            ImGui::PopStyleColor();
        }

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
            ImGui::SetDragDropPayload("MODIFIER_INDEX", &i, sizeof(size_t));
            ImGui::Text("Move %s", header.c_str());
            ImGui::EndDragDropSource();
        }
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MODIFIER_INDEX")) {
                std::size_t sourceIndex = *(const std::size_t*)payload->Data;
                modifierToMove = { sourceIndex, i };
            }
            ImGui::EndDragDropTarget();
        }

        if (isOpen) {
            std::visit([](auto& modifier) {
                modifier.DrawImGui();
            }, this->settings.modifiers[i]);
        }

        if (!keepOpen) {
            modifierToDelete = i;
        }

        ImGui::PopID();
    }

    if (modifierToDelete.has_value()) {
        this->settings.modifiers.erase(this->settings.modifiers.begin() + modifierToDelete.value());
    }

    if (modifierToMove.has_value()) {
        auto [src, dst] = modifierToMove.value();
        auto item = this->settings.modifiers[src];
        this->settings.modifiers.erase(this->settings.modifiers.begin() + src);
        this->settings.modifiers.insert(this->settings.modifiers.begin() + dst, item);
    }

    ImGui::Separator();

    if (ImGui::Button("Add Modifier")) {
        ImGui::OpenPopup("AddModifierPopup");
    }

    if (ImGui::BeginPopup("AddModifierPopup")) {
        if (ImGui::Selectable("Projection Modifier")) { this->settings.modifiers.push_back(ProjectionSettings{}); }
        if (ImGui::Selectable("Relax Modifier")) { this->settings.modifiers.push_back(RelaxSettings{}); }
        if (ImGui::Selectable("Transform Modifier")) { this->settings.modifiers.push_back(TransformSettings{}); }
        if (ImGui::Selectable("Array Modifier")) { this->settings.modifiers.push_back(ArraySettings{}); }
        ImGui::EndPopup();    
    }

    if (this->mesh == nullptr || this->material == nullptr) {
        ImGui::EndDisabled();
    }
}

const char* Spawner::GetModifierName(const ModifierSettings& modifier) {
    if (std::holds_alternative<ProjectionSettings>(modifier)) return "Projection Modifier";
    if (std::holds_alternative<RelaxSettings>(modifier)) return "Relax Modifier";
    if (std::holds_alternative<TransformSettings>(modifier)) return "Transform Modifier";
    if (std::holds_alternative<ArraySettings>(modifier)) return "Array Modifier";
    return "Unknown Modifier";
}
}

