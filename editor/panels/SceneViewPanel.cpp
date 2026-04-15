#include "panels/SceneViewPanel.h"
#include "Application.h"

#include <imgui.h>
#define IMVIEWGUIZMO_IMPLEMENTATION
#include "thirdparty/ImViewGuizmo.h"

#include <Graphics.h>
#include <Scene.h>
#include <TimeSystem.h>
#include <physics/System.h>

#include <algorithm>
#include <glm/gtc/type_ptr.hpp>

namespace Editor {
void SceneViewPanel::Draw(Context& context) {
    ImGui::SetNextWindowSize(ImVec2(1024, 576), ImGuiCond_FirstUseEver);
    ImGui::Begin("Scene View", nullptr, ImGuiWindowFlags_MenuBar);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::RadioButton("Translate", context.currentGizmoOperation ==
                                                ImGuizmo::TRANSLATE)) {
            context.currentGizmoOperation = ImGuizmo::TRANSLATE;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Rotate", context.currentGizmoOperation ==
                                             ImGuizmo::ROTATE)) {
            context.currentGizmoOperation = ImGuizmo::ROTATE;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Scale", context.currentGizmoOperation ==
                                            ImGuizmo::SCALE)) {
            context.currentGizmoOperation = ImGuizmo::SCALE;
        }
        ImGui::EndMenuBar();
    }

    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    float resX = std::max(1.0f, viewportSize.x);
    float resY = std::max(1.0f, viewportSize.y);

    this->HandleMousePicking(context, resX, resY);

    context.selectedScene->GetGraphics()->UpdateScreenResolution(
        glm::vec2(resX, resY));
    context.selectedScene->GetGraphics()->GetMainFramebuffer()->SetSize(
        glm::uvec2(resX, resY));

    // ImGui::ShowDemoWindow();

    Time::Update();
    context.selectedScene->Update();
    context.selectedScene->Render();

    GLuint textureID = context.selectedScene->GetGraphics()
                           ->GetMainFramebuffer()
                           ->GetColorTexture()
                           ->GetHandle();

    ImVec2 cursorScreenPosition = ImGui::GetCursorScreenPos();

    ImGui::Image((ImTextureID)(intptr_t)textureID, ImVec2(resX, resY),
                 ImVec2(0, 1), ImVec2(1, 0));

    if (context.mainCamera != nullptr) {
        if (context.selectedNode != nullptr) {
            ImGuizmo::SetDrawlist();
            ImGuizmo::SetRect(cursorScreenPosition.x, cursorScreenPosition.y,
                              resX, resY);

            glm::mat4 cameraView = context.mainCamera->ViewMatrix();
            glm::mat4 cameraProjection = context.mainCamera->ProjectionMatrix();
            glm::mat4 nodeTransform =
                context.selectedNode->GlobalTransform().Value();

            ImGuizmo::Manipulate(glm::value_ptr(cameraView),
                                 glm::value_ptr(cameraProjection),
                                 context.currentGizmoOperation, ImGuizmo::WORLD,
                                 glm::value_ptr(nodeTransform));

            if (ImGuizmo::IsUsing()) {
                context.selectedNode->GlobalTransform() = nodeTransform;

                if (SceneNode* parent = context.selectedNode->GetParent()) {
                    glm::mat4 parentGlobal = parent->GlobalTransform().Value();
                    glm::mat4 newLocal =
                        glm::inverse(parentGlobal) * nodeTransform;
                    context.selectedNode->LocalTransform() = newLocal;
                } else {
                    context.selectedNode->LocalTransform() = nodeTransform;
                }
            }
        }

        ImViewGuizmo::Style& viewStyle = ImViewGuizmo::GetStyle();
        viewStyle.scale = 0.65f;
        viewStyle.bigCircleColor = IM_COL32(30, 30, 30, 120);

        glm::vec3 cameraPosition =
            context.mainCamera->GlobalTransform().Position();
        glm::quat cameraRotation =
            context.mainCamera->GlobalTransform().Rotation();

        glm::vec3 pivot =
            (context.selectedNode != nullptr)
                ? context.selectedNode->GlobalTransform().Position().Value()
                : glm::vec3(0.0f);

        float gizmoRadius = 128.0f * viewStyle.scale;
        ImVec2 viewGizmoCenter =
            ImVec2(cursorScreenPosition.x + resX - gizmoRadius - 2.0f,
                   cursorScreenPosition.y + gizmoRadius + 2.0f);

        if (ImViewGuizmo::Rotate(cameraPosition, cameraRotation, pivot,
                                 viewGizmoCenter)) {
            context.mainCamera->GlobalTransform().Position() = cameraPosition;
            context.mainCamera->GlobalTransform().Rotation() = cameraRotation;
        }

        float toolButtonSize = viewStyle.toolButtonRadius * viewStyle.scale;
        float spacing = 10.0f;

        ImVec2 panPosition = ImVec2(
            viewGizmoCenter.x - (toolButtonSize * 2.0f) - (spacing / 2.0f),
            viewGizmoCenter.y + gizmoRadius + spacing);

        ImVec2 dollyPosition =
            ImVec2(viewGizmoCenter.x + (spacing / 2.0f),
                   viewGizmoCenter.y + gizmoRadius + spacing);
        if (ImViewGuizmo::Pan(cameraPosition, cameraRotation, panPosition,
                              0.05f)) {
            context.mainCamera->GlobalTransform().Position() = cameraPosition;
        }

        if (ImViewGuizmo::Dolly(cameraPosition, cameraRotation, dollyPosition,
                                0.2f)) {
            context.mainCamera->GlobalTransform().Position() = cameraPosition;
        }
    }

    ImGui::End();
}

void SceneViewPanel::HandleMousePicking(Context& context, float resX,
                                        float resY) {
    if (context.mainCamera != nullptr && ImGui::IsWindowHovered() &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if (!ImGuizmo::IsOver() && !ImViewGuizmo::IsOver()) {
            ImVec2 mousePosition = ImGui::GetMousePos();
            ImVec2 cursorScreenPosition = ImGui::GetCursorScreenPos();

            float mouseX = mousePosition.x - cursorScreenPosition.x;
            float mouseY = mousePosition.y - cursorScreenPosition.y;

            if (mouseX >= 0.0f && mouseX <= resX && mouseY >= 0.0f &&
                mouseY <= resY) {
                float ndcX = (2.0f * mouseX) / resX - 1.0f;
                float ndcY = 1.0f - (2.0f * mouseY) / resY;

                // Move somewhere else
                context.mainCamera->SetAspectRatio(resX / resY);

                glm::mat4 projection = context.mainCamera->ProjectionMatrix();
                glm::mat4 view = context.mainCamera->ViewMatrix();

                glm::vec4 clipSpacePosition(ndcX, ndcY, -1.0f, 1.0f);
                glm::vec4 viewSpacePosition =
                    glm::inverse(projection) * clipSpacePosition;
                viewSpacePosition.z = -1.0f;
                viewSpacePosition.w = 0.0f;

                glm::vec3 rayDirection = glm::normalize(
                    glm::vec3(glm::inverse(view) * viewSpacePosition));
                glm::vec3 rayOrigin =
                    context.mainCamera->GlobalTransform().Position().Value();

                bool hitSomething = false;

                if (Physics::System* physicsSystem =
                        context.selectedScene
                            ->GetComponent<Physics::System>()) {
                    float maxDistance = 1000.0f;
                    glm::vec3 ray = rayDirection * maxDistance;

                    SceneNode* hitNode = physicsSystem->CastRay(
                        rayOrigin, ray, JPH::BroadPhaseLayerFilter(),
                        this->filter);
                    if (hitNode != nullptr) {
                        context.selectedNode = hitNode;
                        hitSomething = true;
                    }
                }

                if (!hitSomething) {
                    context.selectedNode = nullptr;
                }
            }
        }
    }
}

} // namespace Editor
