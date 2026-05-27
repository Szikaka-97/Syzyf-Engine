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
#include "thirdparty/ImGuizmo.h"
#include <Application.h>

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
    Scene* selectedScene = nullptr;
    SceneNode* selectedNode = nullptr;
    Camera* mainCamera = nullptr;
    ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE;

    State state = State::Editor;

    std::unique_ptr<Physics::DebugRenderer> physicsDebugRenderer;
};

class EditorApplication : public ::Application {
  private:
    Settings settings;
    Context context;

    FilesPanel filesPanel;
    ConsolePanel consolePanel;
    MainMenuBar mainMenuBar;
    InspectorPanel inspectorPanel;
    GraphPanel graphPanel;
    SceneViewPanel sceneViewPanel;
    SystemsDebugPanel systemsDebugPanel;
    CommandHistoryPanel commandHistoryPanel;
    StatusBar statusBar;

  public:
    EditorApplication() : ::Application("Syzyf Editor", 1280, 720) {}

  protected:
    void OnInit(int argc = 0, char* argv[] = nullptr) override;
    void OnUpdate() override;
    void OnRender() override;
    void OnImGuiRender() override;
    void OnShutdown() override;

    void ExecutePendingSceneChange() override;

  private:
    void InitSpdlog();
    void Input();
    void DrawPanels();
};
} // namespace Editor
