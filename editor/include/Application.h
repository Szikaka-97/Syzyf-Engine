#pragma once

#include "CommandHistory.h"
#include "Settings.h"
#include "panels/CommandHistoryPanel.h"
#include "panels/ConsolePanel.h"
#include "panels/FilesPanel.h"
#include "panels/GraphPanel.h"
#include "panels/InspectorPanel.h"
#include "panels/MainMenuBar.h"
#include "panels/SceneViewPanel.h"
#include "panels/StatusBar.h"
#include "panels/SystemsDebugPanel.h"
// #include "panels/TextureToolPanel.h"
#include "thirdparty/ImGuizmo.h"

#include <SDL3/SDL_video.h>
#include <physics/DebugRenderer.h>

class Scene;
class SceneNode;
class Camera;

namespace Editor {

enum class State {
    Editor,
    Game,
};

struct Context {
    SDL_Window* window = nullptr;
    SDL_GLContext glContext = nullptr;

    ImFont* consoleFont = nullptr;

    CommandHistory commandHistory;

    std::vector<Scene*> loadedScenes;
    Scene* selectedScene = nullptr; // change to index?
    SceneNode* selectedNode = nullptr;
    Camera* mainCamera = nullptr;
    ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE;

    State state = State::Editor;

    std::unique_ptr<Physics::DebugRenderer> physicsDebugRenderer;
};

class Application {
  private:
    const char* GLSL_VERSION = "#version 460";
    const int32_t GL_VERSION_MAJOR = 4;
    const int32_t GL_VERSION_MINOR = 6;

    Settings settings;
    Context context;

    FilesPanel filesPanel;
    ConsolePanel consolePanel;
    MainMenuBar mainMenuBar;
    InspectorPanel inspectorPanel;
    GraphPanel graphPanel;
    // TextureToolPanel textureToolPanel;
    SceneViewPanel sceneViewPanel;
    SystemsDebugPanel systemsDebugPanel; // Rename
    CommandHistoryPanel commandHistoryPanel;
    StatusBar statusBar;

  public:
    bool Setup();
    void Terminate();
    void MainLoop();

  private:
    void InitSpdlog();
    bool InitProgram();
    bool InitImGui();

    void Input();
    void DrawPanels(bool& shouldClose);
};
} // namespace Editor
