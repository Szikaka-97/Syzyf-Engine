#include "include/Editor.h"
#include "InitScene.h"

#include <imgui.h>
#define IMVIEWGUIZMO_IMPLEMENTATION
#include "thirdparty/ImGuizmo.h"
#include "thirdparty/ImViewGuizmo.h"

#include <Jolt/RegisterTypes.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>
#include <algorithm>
#include <filesystem>
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_impl/imgui_impl_opengl3.h>
#include <imgui_impl/imgui_impl_sdl3.h>
#include <spdlog/spdlog.h>

#include <Engine.h>
#include <Graphics.h>
#include <physics/Jolt.h>
#include <physics/System.h>

namespace Editor {
const char* GLSL_VERSION = "#version 460";
constexpr int32_t GL_VERSION_MAJOR = 4;
constexpr int32_t GL_VERSION_MINOR = 6;

SDL_Window* window = nullptr;
SDL_GLContext glContext = nullptr;

// Move into some struct
SceneNode* selectedNode = nullptr;
Camera* mainCamera = nullptr;
ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE;

bool InitProgram() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        spdlog::error("Failed to initialize SDL3: {}", SDL_GetError());
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, GL_VERSION_MAJOR);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, GL_VERSION_MINOR);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);

    window = SDL_CreateWindow("Syzyf Editor", 1280, 720,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        spdlog::error("Failed to create window: {}", SDL_GetError());
        return false;
    }

    glContext = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, glContext);
    SDL_GL_SetSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        spdlog::error("Failed to initialize GLAD");
        return false;
    }

    Engine::window = window;

    JPH::RegisterDefaultAllocator();
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();

    JPH::Trace = Physics::TraceImpl;
#ifdef JPH_ENABLE_ASSERTS
    JPH::AssertFailed = Physics::AssertFailedImpl;
#endif

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    return true;
}

bool InitImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    io.IniFilename = "imgui.ini";

    if (!std::filesystem::exists("imgui.ini")) {
        if (std::filesystem::exists("default_editor_layout.ini")) {
            ImGui::LoadIniSettingsFromDisk("default_editor_layout.ini");
        }
    }

    // Add a toggle, add a custom
    // ImGui::StyleColorsDark();
    ImGui::StyleColorsLight();

    ImGui_ImplSDL3_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL3_Init(GLSL_VERSION);

    return true;
}

bool Setup() { return InitProgram() && InitImGui(); }

void Terminate() {
    ImGui::DestroyPlatformWindows();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void DrawMainMenuBar(bool& shouldClose) {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit")) {
                shouldClose = true;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            if (ImGui::BeginMenu("Theme")) {
                if (ImGui::MenuItem("Dark")) {
                    ImGui::StyleColorsDark();
                }
                if (ImGui::MenuItem("Light")) {
                    ImGui::StyleColorsLight();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void DrawGraphNode(SceneNode& node) {
    ImGui::PushID(node.GetID());

    ImGui::TableNextRow();
    ImGui::TableNextColumn();

    std::string treeHeader = node.GetName();
    if (treeHeader.empty()) {
        treeHeader = std::to_string(node.GetID());
    }

    bool isLeaf = node.GetChildren().empty();

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (&node == node.GetScene()->GetRootNode()) {
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }

    if (selectedNode == &node) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    if (isLeaf) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    bool nodeOpen = ImGui::TreeNodeEx((void*)(intptr_t)node.GetID(), flags,
                                      "%s", treeHeader.c_str());

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        selectedNode = &node;
    }

    ImGui::TableNextColumn();

    const bool isEnabled = node.IsEnabled();

    if (!isEnabled) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                              ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                              ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
    }

    if (ImGui::Button("E", ImVec2(24, ImGui::GetFrameHeight()))) {
        node.SetEnabled(!node.IsEnabled());
    }

    if (!isEnabled)
        ImGui::PopStyleColor(3);

    if (nodeOpen) {
        if (!isLeaf) {
            for (SceneNode* child : node.GetChildren()) {
                DrawGraphNode(*child);
            }
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}

void DrawGraph(Scene& scene) {
    ImGui::Begin("Graph");

    SceneNode* root = scene.GetRootNode();

    if (root != nullptr) {
        ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 10.0f);

        ImGuiTableFlags tableFlags =
            ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
            ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoBordersInBody;

        if (ImGui::BeginTable("Graph Table", 2, tableFlags)) {
            ImGui::TableSetupColumn("Node", ImGuiTableColumnFlags_WidthStretch);

            // change so the 30px isnt hardcoded
            ImGui::TableSetupColumn("Visibility",
                                    ImGuiTableColumnFlags_WidthFixed, 30.0f);

            DrawGraphNode(*root);

            ImGui::EndTable();
        }
        ImGui::PopStyleVar();
    }

    ImGui::End();
}

void DrawInspector() {
    ImGui::Begin("Inspector");
    if (selectedNode != nullptr) {
        std::string name = selectedNode->GetName();
        if (name.empty()) {
            ImGui::TextUnformatted(
                std::to_string(selectedNode->GetID()).c_str());
        } else {
            ImGui::TextUnformatted(name.c_str());
        }

        bool nodeEnabled = selectedNode->IsEnabled();
        ImGui::Checkbox("Enabled", &nodeEnabled);
        selectedNode->SetEnabled(nodeEnabled);

        if (ImGui::TreeNode("Layer")) {
            const float size = ImGui::CalcTextSize("00").x;

            for (int y = 0; y < 4; y++) {
                for (int x = 0; x < 8; x++) {
                    if (x > 0) {
                        ImGui::SameLine();
                    }

                    uint8_t layer = y * 8 + x;

                    ImGui::PushID(layer);

                    if (ImGui::Selectable(std::to_string(layer).c_str(),
                                          selectedNode->GetLayer() == layer, 0,
                                          ImVec2(size, size))) {
                        selectedNode->SetLayer(layer);
                    }

                    ImGui::PopID();
                }
            }

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Transform")) {
            ImGui::Text("Position");

            glm::vec3 position = selectedNode->GlobalTransform().Position();

            ImGui::InputFloat3("##Position", &position[0]);

            glm::vec3 positionDelta = glm::zero<glm::vec3>();

            ImGui::SliderFloat3("##PositionDelta", &positionDelta[0], -1, 1);

            position += positionDelta;

            selectedNode->GlobalTransform().Position() = position;

            ImGui::Text("Rotation");

            glm::vec3 rotationEuler = glm::degrees(glm::eulerAngles(
                selectedNode->GlobalTransform().Rotation().Value()));

            ImGui::InputFloat3("##Rotation", &rotationEuler[0]);

            selectedNode->GlobalTransform().Rotation() =
                glm::quat(glm::radians(rotationEuler));

            glm::vec3 rotationDelta = glm::zero<glm::vec3>();

            ImGui::SliderFloat3("##RotationDelta", &rotationDelta[0], -1, 1);

            selectedNode->GlobalTransform().Rotation() *=
                glm::angleAxis(glm::radians(rotationDelta.x),
                               glm::vec3(1, 0, 0)) *
                glm::angleAxis(glm::radians(rotationDelta.y),
                               glm::vec3(0, 1, 0)) *
                glm::angleAxis(glm::radians(rotationDelta.z),
                               glm::vec3(0, 0, 1));

            ImGui::Text("Scale");

            glm::vec3 scale = selectedNode->GlobalTransform().Scale();

            ImGui::InputFloat3("##Scale", &scale[0]);

            glm::vec3 scaleDelta = glm::zero<glm::vec3>();

            ImGui::SliderFloat3("##ScaleDelta", &scaleDelta[0], -1, 1);

            scale += scaleDelta;

            if (glm::abs(scale.x) < 0.0001) {
                scale.x = 0.0001;
            }
            if (glm::abs(scale.y) < 0.0001) {
                scale.y = 0.0001;
            }
            if (glm::abs(scale.z) < 0.0001) {
                scale.z = 0.0001;
            }

            selectedNode->GlobalTransform().Scale() = scale;

            ImGui::TreePop();
        }

        AnimationComponent* animationComponent =
            selectedNode->GetObject<AnimationComponent>();
        if (animationComponent != nullptr) {
            if (ImGui::TreeNode("Animation")) {
                for (auto& animation : animationComponent->animations) {
                    if (ImGui::TreeNode(animation.data.name.c_str())) {
                        ImGui::Text("%s", std::format("Duration: {}",
                                                      animation.data.duration)
                                              .c_str());
                        ImGui::Text("%s", std::format("Progress: {}",
                                                      animation.timeActive)
                                              .c_str());
                        ImGui::Checkbox("Playing", &animation.playing);
                        ImGui::Checkbox("Looping", &animation.looping);
                        ImGui::DragFloat("Speed", &animation.speed, 1.0f, 0.0f,
                                         5.0f, "%.2f");
                        // animation.data.tracks.front().inputs add this maybe
                        ImGui::TreePop();
                    }
                }
                ImGui::TreePop();
            };
        }

        int index = 0;
        for (GameObject* obj : selectedNode->AttachedObjects()) {
            ImGui::PushID(obj->GetID());
            if (ImGui::TreeNode(
                    std::format("{}: {}", index, obj->GetName()).c_str())) {
                ImGui::Text("Object ID: %i", obj->GetID());

                bool objEnabled = obj->IsEnabled();

                ImGui::Checkbox("Enabled", &objEnabled);

                obj->SetEnabled(objEnabled);

                ImGuiDrawable* imguiObj = dynamic_cast<ImGuiDrawable*>(obj);

                if (imguiObj) {
                    ImGui::Separator();

                    imguiObj->DrawImGui();
                }
                ImGui::TreePop();
            }
            index++;
            ImGui::PopID();
        }
    }
    ImGui::End();
}

void DrawFiles() {
    ImGui::Begin("Files");
    ImGui::End();
}

void HandleMousePicking(Scene& scene, float resX, float resY) {
    if (mainCamera != nullptr && ImGui::IsWindowHovered() &&
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
                mainCamera->SetAspectRatio(resX / resY);

                glm::mat4 projection = mainCamera->ProjectionMatrix();
                glm::mat4 view = mainCamera->ViewMatrix();

                glm::vec4 clipSpacePosition(ndcX, ndcY, -1.0f, 1.0f);
                glm::vec4 viewSpacePosition =
                    glm::inverse(projection) * clipSpacePosition;
                viewSpacePosition.z = -1.0f;
                viewSpacePosition.w = 0.0f;

                glm::vec3 rayDirection = glm::normalize(
                    glm::vec3(glm::inverse(view) * viewSpacePosition));
                glm::vec3 rayOrigin =
                    mainCamera->GlobalTransform().Position().Value();

                bool hitSomething = false;

                if (Physics::System* physicsSystem =
                        scene.GetComponent<Physics::System>()) {
                    float maxDistance = 1000.0f;
                    glm::vec3 ray = rayDirection * maxDistance;

                    SceneNode* hitNode = physicsSystem->CastRay(rayOrigin, ray);
                    if (hitNode != nullptr) {
                        selectedNode = hitNode;
                        hitSomething = true;
                    }
                }

                if (!hitSomething) {
                    // Add Bounds fallback

                    selectedNode = nullptr;
                }
            }
        }
    }
}

void DrawSceneView(Scene& scene) {
    ImGui::SetNextWindowSize(ImVec2(1024, 576), ImGuiCond_FirstUseEver);
    ImGui::Begin("Scene View", nullptr, ImGuiWindowFlags_MenuBar);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::RadioButton("Translate",
                               currentGizmoOperation == ImGuizmo::TRANSLATE)) {
            currentGizmoOperation = ImGuizmo::TRANSLATE;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Rotate",
                               currentGizmoOperation == ImGuizmo::ROTATE)) {
            currentGizmoOperation = ImGuizmo::ROTATE;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Scale",
                               currentGizmoOperation == ImGuizmo::SCALE)) {
            currentGizmoOperation = ImGuizmo::SCALE;
        }
        ImGui::EndMenuBar();
    }

    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    float resX = std::max(1.0f, viewportSize.x);
    float resY = std::max(1.0f, viewportSize.y);

    Editor::HandleMousePicking(scene, resX, resY);

    scene.GetGraphics()->UpdateScreenResolution(glm::vec2(resX, resY));
    scene.GetGraphics()->GetMainFramebuffer()->SetSize(glm::uvec2(resX, resY));

    // ImGui::ShowDemoWindow();

    Time::Update();
    scene.Update();
    scene.Render();

    GLuint textureID = scene.GetGraphics()
                           ->GetMainFramebuffer()
                           ->GetColorTexture()
                           ->GetHandle();

    ImVec2 cursorScreenPosition = ImGui::GetCursorScreenPos();

    ImGui::Image((ImTextureID)(intptr_t)textureID, ImVec2(resX, resY),
                 ImVec2(0, 1), ImVec2(1, 0));

    if (mainCamera != nullptr) {
        if (selectedNode != nullptr) {
            ImGuizmo::SetDrawlist();
            ImGuizmo::SetRect(cursorScreenPosition.x, cursorScreenPosition.y,
                              resX, resY);

            glm::mat4 cameraView = mainCamera->ViewMatrix();
            glm::mat4 cameraProjection = mainCamera->ProjectionMatrix();
            glm::mat4 nodeTransform = selectedNode->GlobalTransform().Value();

            ImGuizmo::Manipulate(glm::value_ptr(cameraView),
                                 glm::value_ptr(cameraProjection),
                                 currentGizmoOperation, ImGuizmo::WORLD,
                                 glm::value_ptr(nodeTransform));

            if (ImGuizmo::IsUsing()) {
                selectedNode->GlobalTransform() = nodeTransform;
            }
        }

        ImViewGuizmo::Style& viewStyle = ImViewGuizmo::GetStyle();
        viewStyle.scale = 0.65f;
        viewStyle.bigCircleColor = IM_COL32(30, 30, 30, 120);

        glm::vec3 cameraPosition = mainCamera->GlobalTransform().Position();
        glm::quat cameraRotation = mainCamera->GlobalTransform().Rotation();

        glm::vec3 pivot =
            (selectedNode != nullptr)
                ? selectedNode->GlobalTransform().Position().Value()
                : glm::vec3(0.0f);

        float gizmoRadius = 128.0f * viewStyle.scale;
        ImVec2 viewGizmoCenter =
            ImVec2(cursorScreenPosition.x + resX - gizmoRadius - 2.0f,
                   cursorScreenPosition.y + gizmoRadius + 2.0f);

        if (ImViewGuizmo::Rotate(cameraPosition, cameraRotation, pivot,
                                 viewGizmoCenter)) {
            mainCamera->GlobalTransform().Position() = cameraPosition;
            mainCamera->GlobalTransform().Rotation() = cameraRotation;
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
            mainCamera->GlobalTransform().Position() = cameraPosition;
        }

        if (ImViewGuizmo::Dolly(cameraPosition, cameraRotation, dollyPosition,
                                0.2f)) {
            mainCamera->GlobalTransform().Position() = cameraPosition;
        }
    }

    ImGui::End();
}

void MainLoop() {
    // temporary
    std::unique_ptr<Scene> scene(Scene::CreateStandaloneScene());
    InitScene(*scene, mainCamera);

    scene->GetGraphics()->UpdateScreenResolution(glm::vec2(1024.0f, 576.0f));

    bool shouldClose = false;
    while (!shouldClose) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                shouldClose = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                event.window.windowID == SDL_GetWindowID(window))
                shouldClose = true;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();
        ImViewGuizmo::BeginFrame();

        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);

        Editor::DrawMainMenuBar(shouldClose);
        Editor::DrawGraph(*scene);
        Editor::DrawInspector();
        Editor::DrawFiles();
        Editor::DrawSceneView(*scene);

        ImGui::Render();
        ImGuiIO& io = ImGui::GetIO();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }
}
} // namespace Editor
