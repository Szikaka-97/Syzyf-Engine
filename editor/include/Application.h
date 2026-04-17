#pragma once

#include "Settings.h"
#include "panels/ConsolePanel.h"
#include "panels/FilesPanel.h"
#include "panels/GraphPanel.h"
#include "panels/InspectorPanel.h"
#include "panels/MainMenuBar.h"
#include "panels/SceneViewPanel.h"

#include "panels/SystemsDebugPanel.h"
#include "thirdparty/ImGuizmo.h"
#include <SDL3/SDL_video.h>
class Scene;
class SceneNode;
class Camera;

namespace Editor {

struct Context {
    Scene* selectedScene = nullptr;
    SceneNode* selectedNode = nullptr;
    Camera* mainCamera = nullptr;
    ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE;
};

class Application {
  private:
    const char* GLSL_VERSION = "#version 460";
    const int32_t GL_VERSION_MAJOR = 4;
    const int32_t GL_VERSION_MINOR = 6;

    SDL_Window* window = nullptr;
    SDL_GLContext glContext = nullptr;

    Settings settings;
    Context context;

    FilesPanel filesPanel;
    ConsolePanel consolePanel;
    MainMenuBar mainMenuBar;
    InspectorPanel inspectorPanel;
    GraphPanel graphPanel;
    SceneViewPanel sceneViewPanel;
    SystemsDebugPanel systemsDebugPanel; // Rename

  public:
    bool Setup();
    void Terminate();
    void MainLoop();

  private:
    void InitSpdlog();
    bool InitProgram();
    bool InitImGui();
};
} // namespace Editor
