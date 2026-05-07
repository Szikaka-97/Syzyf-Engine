#include "panels/SceneViewPanel.h"
#include "AiNode.h"
#include "Application.h"
#include "CameraController.h"
#include "Commands.h"
#include "MousePickingBodySystem.h"
#include "ParticleSpawner.h"
#include "physics/Body.h"
#include "physics/DebugRenderer.h"

#include "physics/CharacterController.h"
#include "physics/VirtualCharacterController.h"
#include "scatter/Spawner.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_mouse.h>
#include <glm/glm.hpp>
#include <imgui.h>
#define IMVIEWGUIZMO_IMPLEMENTATION
#include "thirdparty/ImViewGuizmo.h"

#include <Graphics.h>
#include <MeshRenderer.h>
#include <Scene.h>
#include <TimeSystem.h>
#include <physics/System.h>

#include <algorithm>
#include <glm/gtc/type_ptr.hpp>

namespace Editor {
void SceneViewPanel::Draw(Context& context) {
    bool drawContent = false;
    bool startedAsPopup = this->isFullscreen;

    if (startedAsPopup) {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);

        if (!ImGui::IsPopupOpen("Fullscreen Scene View")) {
            ImGui::OpenPopup("Fullscreen Scene View");
        }

        ImGuiWindowFlags popupFlags =
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;
        if (ImGui::BeginPopupModal("Fullscreen Scene View", nullptr,
                                   popupFlags)) {
            drawContent = true;

            if (ImGui::IsKeyPressed(ImGuiKey_F11)) {
                this->isFullscreen = false;
                this->isBarHidden = false;
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::IsKeyPressed(ImGuiKey_LeftAlt) ||
                ImGui::IsKeyPressed(ImGuiKey_RightAlt)) {
                this->isBarHidden = !this->isBarHidden;
            }
        }

        ImGui::Begin("Scene View", nullptr);
        ImGui::TextDisabled("Scene View is currently in Fullscreen mode.");
        if (ImGui::Button("Exit Fullscreen (F11)")) {
            this->isFullscreen = false;
            this->isBarHidden = false;
        }
        ImGui::End();
    } else {
        ImGui::SetNextWindowSize(ImVec2(1024, 576), ImGuiCond_FirstUseEver);
        drawContent = ImGui::Begin("Scene View", nullptr);

        if (ImGui::IsKeyPressed(ImGuiKey_F11) &&
            ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
            this->isFullscreen = true;
        }
    }

    if (drawContent) {
        if (context.loadedScenes.empty()) {
            if (startedAsPopup)
                ImGui::EndPopup();
            else
                ImGui::End();
            return;
        }

        if (!this->isBarHidden) {
            if (context.state == State::Game) {
                ImGui::BeginDisabled();
            }
            if (ImGui::BeginTabBar("SceneTabBar", ImGuiTabBarFlags_None)) {
                for (std::size_t i = 0; i < context.loadedScenes.size(); ++i) {
                    Scene* scene = context.loadedScenes[i];

                    std::string tabName = "Scene " + std::to_string(i + 1);

                    if (ImGui::BeginTabItem(tabName.c_str())) {
                        if (context.selectedScene != scene) {
                            context.selectedScene = scene;

                            context.selectedNode = nullptr;
                            context.mainCamera =
                                scene->FindObjectsOfType<CameraController>()
                                    .front()
                                    ->GetObject<Camera>();
                            context.mainCamera->SetAsMainCamera();
                        }
                        ImGui::EndTabItem();
                    }
                }
                ImGui::EndTabBar();
            }
            if (context.state == State::Game) {
                ImGui::EndDisabled();
            }
        }

        if (context.selectedScene == nullptr) {
            if (startedAsPopup)
                ImGui::EndPopup();
            else
                ImGui::End();
            return;
        }
    }

    if (!this->isBarHidden) {
        this->DrawMenuBar(context);
    }

    ImVec2 cursorScreenPosition = ImGui::GetCursorScreenPos();

    if (auto* inputSystem =
            context.selectedScene->GetComponent<InputSystem>()) {
        inputSystem->SetViewportOffset(
            glm::vec2(cursorScreenPosition.x, cursorScreenPosition.y));
    }

    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    float resX = std::max(1.0f, viewportSize.x);
    float resY = std::max(1.0f, viewportSize.y);

    this->HandleMousePicking(context, resX, resY);

    context.selectedScene->GetGraphics()->UpdateScreenResolution(
        glm::vec2(resX, resY));
    context.selectedScene->GetGraphics()->GetMainFramebuffer()->SetSize(
        glm::uvec2(resX, resY));

    Time::Update();

    this->UpdateAndRenderScene(context);

    GLuint textureID = context.selectedScene->GetGraphics()
                           ->GetMainFramebuffer()
                           ->GetColorTexture()
                           ->GetHandle();

    cursorScreenPosition = ImGui::GetCursorScreenPos();

    ImGui::Image((ImTextureID)(intptr_t)textureID, ImVec2(resX, resY),
                 ImVec2(0, 1), ImVec2(1, 0));

    if (ImGui::BeginDragDropTarget()) {
        this->HandleDrop(context);
    }

    if (context.state != State::Game && context.mainCamera != nullptr) {
        if (context.selectedNode != nullptr) {
            if ((ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) ||
                 this->keyboardControls.IsActive()) &&
                !SDL_GetWindowRelativeMouseMode(context.window)) {
                this->keyboardControls.Run(context);

                if (ImGui::Shortcut(ImGuiKey_Z)) {
                    context.currentGizmoOperation = ImGuizmo::TRANSLATE;
                }
                if (ImGui::Shortcut(ImGuiKey_X)) {
                    context.currentGizmoOperation = ImGuizmo::ROTATE;
                }
                if (ImGui::Shortcut(ImGuiKey_C)) {
                    context.currentGizmoOperation = ImGuizmo::SCALE;
                }

                if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
                    // context.selectedScene->DeleteNode(context.selectedNode);
                    // context.selectedNode = nullptr;

                    // TODO uncomment when deleting doesnt crash the game
                }
            }

            ImGuizmo::SetDrawlist();
            ImGuizmo::SetRect(cursorScreenPosition.x, cursorScreenPosition.y,
                              resX, resY);

            glm::mat4 cameraView = context.mainCamera->ViewMatrix();
            glm::mat4 cameraProjection = context.mainCamera->ProjectionMatrix();
            glm::mat4 nodeTransform =
                context.selectedNode->GlobalTransform().Value();

            bool isSnapping = ImGui::IsKeyDown(ImGuiKey_LeftCtrl);
            float snapValue =
                (context.currentGizmoOperation == ImGuizmo::ROTATE) ? 45.0f
                                                                    : 1.0f;
            std::array<float, 3> snapValues{snapValue, snapValue, snapValue};
            ImGuizmo::Manipulate(
                glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
                context.currentGizmoOperation,
                this->isGizmoLocal ? ImGuizmo::LOCAL : ImGuizmo::WORLD,
                glm::value_ptr(nodeTransform), nullptr,
                isSnapping ? snapValues.data() : nullptr);

            if (ImGuizmo::IsUsing()) {
                if (this->wasViewGuizmoUsed == false) {
                    this->wasViewGuizmoUsed = true;
                    this->initialLocalTransform =
                        context.selectedNode->LocalTransform().Value();
                }

                context.selectedNode->GlobalTransform() = nodeTransform;

                if (SceneNode* parent = context.selectedNode->GetParent()) {
                    glm::mat4 parentGlobal = parent->GlobalTransform().Value();
                    glm::mat4 newLocal =
                        glm::inverse(parentGlobal) * nodeTransform;
                    context.selectedNode->LocalTransform() = newLocal;
                } else {
                    context.selectedNode->LocalTransform() = nodeTransform;
                }
            } else if (this->wasViewGuizmoUsed) {
                this->wasViewGuizmoUsed = false;

                context.commandHistory.ExecuteCommand(
                    std::make_unique<TransformCommand>(
                        context.selectedNode, this->initialLocalTransform,
                        context.selectedNode->LocalTransform().Value()));
            }
        }

        if (!ImGuizmo::IsUsing()) { // Prevents accidental presses when using
                                    // the other gizmo
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
                context.mainCamera->GlobalTransform().Position() =
                    cameraPosition;
                context.mainCamera->GlobalTransform().Rotation() =
                    cameraRotation;
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
                context.mainCamera->GlobalTransform().Position() =
                    cameraPosition;
            }

            if (ImViewGuizmo::Dolly(cameraPosition, cameraRotation,
                                    dollyPosition, 0.2f)) {
                context.mainCamera->GlobalTransform().Position() =
                    cameraPosition;
            }
        }
    }

    if (startedAsPopup) {
        ImGui::EndPopup();
    } else {
        ImGui::End();
    }
}

void SceneViewPanel::UpdateAndRenderScene(Context& context) {
    if (context.state == State::Game) {
        context.selectedScene->Update();
    } else {
        context.selectedScene->GetComponent<InputSystem>()->OnPreUpdate();

        for (auto* body :
             context.selectedScene->FindObjectsOfType<Physics::Body>()) {
            body->SyncToNode();
        }
        for (auto* controller :
             context.selectedScene
                 ->FindObjectsOfType<Physics::CharacterController>()) {
            controller->SyncToNode();
        }
        for (auto* controller :
             context.selectedScene
                 ->FindObjectsOfType<Physics::VirtualCharacterController>()) {
            controller->SyncToNode();
        }

        for (auto* spawner :
             context.selectedScene->FindObjectsOfType<ParticleSpawner>()) {
            spawner->Update();
        }

        for (auto* spawner :
             context.selectedScene->FindObjectsOfType<Scatter::Spawner>()) {
            spawner->Update();
        }

        context.selectedScene->GetComponent<MousePickingBodySystem>()
            ->OnPreUpdate();
        context.selectedScene->FindObjectsOfType<CameraController>()
            .front()
            ->Update();
        context.selectedScene->GetComponent<InputSystem>()->OnPostUpdate();
    }
    context.selectedScene->Render();
    if (context.state != State::Game) {
        for (auto* aiNode :
             context.selectedScene->FindObjectsOfType<AiNode>()) {
            aiNode->DrawDebugView(context.physicsDebugRenderer.get());
        }

        context.physicsDebugRenderer->Render();
    }

    if (auto* physicsSystem =
            context.selectedScene->GetComponent<Physics::System>()) {
        physicsSystem->DrawPhysicsDebug(context.physicsDebugRenderer.get());
        glBindFramebuffer(GL_FRAMEBUFFER, context.selectedScene->GetGraphics()
                                              ->GetMainFramebuffer()
                                              ->GetHandle());
        context.physicsDebugRenderer->Render();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

void SceneViewPanel::HandleMousePicking(Context& context, float resX,
                                        float resY) {
    // Could allow for mousepicking in game later
    if (context.state == State::Editor && context.mainCamera != nullptr &&
        ImGui::IsWindowHovered() &&
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

                    Physics::RayCastPayload hit = physicsSystem->CastRay(
                        rayOrigin, ray, JPH::BroadPhaseLayerFilter(),
                        this->filter);
                    SceneNode* hitNode = hit.node;

                    if (hitNode != nullptr) {
                        context.selectedNode = hitNode;
                        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl)) {
                            while (context.selectedNode != nullptr &&
                                   context.selectedNode->GetParent() !=
                                       nullptr &&
                                   context.selectedNode->GetParent() !=
                                       context.selectedScene->GetRootNode()) {

                                context.selectedNode =
                                    context.selectedNode->GetParent();
                            }
                        }
                    }
                }
            }
        }
    }
}

void SceneViewPanel::HandleDrop(Context& context) {
    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(
            "DND_FILE_PATH", ImGuiDragDropFlags_AcceptBeforeDelivery)) {

        const char* droppedFilePath = (const char*)payload->Data;
        std::string filePathStr(droppedFilePath);
        std::filesystem::path droppedPath(filePathStr);
        std::string normalizedPath = droppedPath.generic_string();

        glm::vec3 spawnPosition(0.0f);
        bool hasValidSpawnPosition = false;

        ImVec2 mousePosition = ImGui::GetMousePos();

        ImVec2 viewportMin = ImGui::GetItemRectMin();
        ImVec2 viewportSize = ImGui::GetItemRectSize();

        float resX = std::max(1.0f, viewportSize.x);
        float resY = std::max(1.0f, viewportSize.y);

        float mouseX = mousePosition.x - viewportMin.x;
        float mouseY = mousePosition.y - viewportMin.y;

        if (context.mainCamera != nullptr && mouseX >= 0.0f && mouseX <= resX &&
            mouseY >= 0.0f && mouseY <= resY) {
            float ndcX = (2.0f * mouseX) / resX - 1.0f;
            float ndcY = 1.0f - (2.0f * mouseY) / resY;

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

            if (auto* physicsSystem =
                    context.selectedScene->GetComponent<Physics::System>()) {
                float maxDistance = 1000.0f;
                glm::vec3 rayExtent = rayDirection * maxDistance;

                Physics::RayCastPayload hit = physicsSystem->CastRay(
                    rayOrigin, rayExtent, JPH::BroadPhaseLayerFilter(),
                    this->filter);

                if (hit.hasHit) {
                    spawnPosition = hit.position;
                    hasValidSpawnPosition = true;
                }
            }

            if (!hasValidSpawnPosition && rayDirection.y < 0.0f) {
                float t = -rayOrigin.y / rayDirection.y;
                spawnPosition = rayOrigin + rayDirection * t;
                hasValidSpawnPosition = true;
            }
        }

        // TODO make it so you can see the model when previewing
        if (payload->IsPreview()) {
            if (hasValidSpawnPosition) {
                ImGui::SetTooltip("Drop to spawn at: (%.1f, %1.f, %1.f)",
                                  spawnPosition.x, spawnPosition.y,
                                  spawnPosition.z);
            }
        }

        if (payload->IsDelivery()) {
            if (droppedPath.extension() == ".glb" ||
                droppedPath.extension() == ".gltf") {
                SceneNode* node = GltfImporter::LoadScene(
                    context.selectedScene, normalizedPath.c_str());

                if (hasValidSpawnPosition) {
                    node->LocalTransform().Position() = spawnPosition;
                }
                context.selectedNode = node;
            }
            if (droppedPath.extension() == ".obj" ||
                droppedPath.extension() == ".fbx") {
                if (Mesh* mesh = context.selectedScene->Resources()->Get<Mesh>(
                        normalizedPath, true)) {
                    SceneNode* modelNode = context.selectedScene->CreateNode();

                    if (hasValidSpawnPosition) {
                        modelNode->LocalTransform().Position() = spawnPosition;
                    }

                    if (mesh->GetDefaultMaterials().empty()) {
                        ShaderProgram* defaultProg =
                            ShaderProgram::Build()
                                .WithVertexShader("./res/shaders/lit.vert")
                                .WithPixelShader(
                                    "./res/shaders/lambert color.frag")
                                .Link();

                        auto* defaultMaterial = new Material(defaultProg);
                        defaultMaterial->SetValue("uColor", glm::vec3(0.8));
                        modelNode->AddObject<MeshRenderer>(mesh,
                                                           defaultMaterial);
                    } else {
                        modelNode->AddObject<MeshRenderer>(
                            mesh, mesh->GetDefaultMaterials());
                    }

                    context.selectedNode = modelNode;
                } else {
                    spdlog::error(
                        "Editor::SceneViewPanel::HandleDrop: Failed to "
                        "load an .obj or .fbx file");
                }
            }
        }
    }
    ImGui::EndDragDropTarget();
}

void SceneViewPanel::DrawMenuBar(Context& context) {
    if (ImGui::RadioButton("Translate", context.currentGizmoOperation ==
                                            ImGuizmo::TRANSLATE)) {
        context.currentGizmoOperation = ImGuizmo::TRANSLATE;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Rotate",
                           context.currentGizmoOperation == ImGuizmo::ROTATE)) {
        context.currentGizmoOperation = ImGuizmo::ROTATE;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Scale",
                           context.currentGizmoOperation == ImGuizmo::SCALE)) {
        context.currentGizmoOperation = ImGuizmo::SCALE;
    }

    // Local/Global Gizmo Mode
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    if (ImGui::RadioButton("World", !this->isGizmoLocal)) {
        this->isGizmoLocal = false;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Local", this->isGizmoLocal)) {
        this->isGizmoLocal = true;
    }

    ImGuiStyle& style = ImGui::GetStyle();

    float buttonWidth =
        ImGui::CalcTextSize("<").x + style.FramePadding.x * 2.0f;

    float editorWidth = ImGui::GetFrameHeight() + style.ItemInnerSpacing.x +
                        ImGui::CalcTextSize("Editor").x;
    float gameWidth = ImGui::GetFrameHeight() + style.ItemInnerSpacing.x +
                      ImGui::CalcTextSize("Game").x;
    float fullScreenBtnWidth =
        ImGui::CalcTextSize("Fullscreen").x + style.FramePadding.x * 2.0f;
    float rightPadding = 10.0f;

    float totalWidth = editorWidth + style.ItemSpacing.x + gameWidth +
                       style.ItemSpacing.x + buttonWidth + style.ItemSpacing.x +
                       buttonWidth + style.ItemSpacing.x + fullScreenBtnWidth +
                       rightPadding;

    ImGui::SameLine(ImGui::GetWindowWidth() - totalWidth);

    if (ImGui::Button(this->isFullscreen ? "Minimize" : "Fullscreen")) {
        this->isFullscreen = !this->isFullscreen;
        if (!this->isFullscreen) {
            ImGui::CloseCurrentPopup();
        }
    }
    ImGui::SameLine();

    if (ImGui::RadioButton("Editor", context.state == State::Editor)) {
        if (context.state != State::Editor) {
            context.state = State::Editor;
            context.mainCamera =
                context.selectedScene->FindObjectsOfType<CameraController>()
                    .front()
                    ->GetObject<Camera>();
            context.mainCamera->SetAsMainCamera();
        }
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Game", context.state == State::Game)) {
        if (context.state != State::Game) {
            context.state = State::Game;

            bool changedCamera = false;
            for (auto* camera :
                 context.selectedScene->FindObjectsOfType<Camera>()) {
                if (camera != context.mainCamera) {
                    context.mainCamera = camera;
                    context.mainCamera->SetAsMainCamera();
                    break;
                } else {
                    spdlog::error("Editor: No camera was added to the scene");
                }
            }
        }
    }

    ImGui::SameLine();
    if (context.commandHistory.CanUndo()) {
        if (ImGui::Button("<")) {
            context.commandHistory.Undo();
        }
    } else {
        ImGui::BeginDisabled();
        ImGui::Button("<");
        ImGui::EndDisabled();
    }

    ImGui::SameLine();
    if (context.commandHistory.CanRedo()) {
        if (ImGui::Button(">")) {
            context.commandHistory.Redo();
        }
    } else {
        ImGui::BeginDisabled();
        ImGui::Button(">");
        ImGui::EndDisabled();
    }
}

} // namespace Editor
