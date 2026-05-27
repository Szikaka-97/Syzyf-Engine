#pragma once

#include "Settings.h"
#include <SDL3/SDL_video.h>

#include <functional>
#include <string>

class Scene;

using SceneInitCallback = std::function<void(class Scene*)>;

class Application {
protected:
    static Application* instance;

    SDL_Window* window = nullptr;
    SDL_GLContext glContext = nullptr;
    Scene* currentScene = nullptr;
    
    int windowWidth = 1280;
    int windowHeight = 720;
    std::string windowTitle = "Syzyf Engine";

    bool isRunning = true;

    bool isSceneChangeRequested = false;
    Scene* pendingScene = nullptr;
    SceneInitCallback pendingSceneInitFunc = nullptr;

    GameSettings settings;

public:
    Application(const std::string& title = "Syzyf", int width = 1280, int height = 720);
    virtual ~Application();

    void Run(int argc = 0, char* argv[] = nullptr);

    GameSettings& GetSettings() { return settings; }
    virtual void ApplySettings() {}

    static SDL_Window* GetWindow();
    static Scene* GetCurrentScene();
    static Application* Get();

    // Request a scene change to a scene defined using a function
    void RequestSceneBuild(SceneInitCallback initFunc);
    // Request a scene change to an already instantiated function
    void RequestSceneChange(Scene* scene);

protected:
    virtual void OnInit(int argc = 0, char* argv[] = nullptr) {}
    virtual void OnUpdate() {}
    virtual void OnRender() {}
    virtual void OnImGuiRender() {}
    virtual void OnShutdown() {}

    virtual void ExecutePendingSceneChange();

private:
    bool InitEngine();
    void ShutdownEngine();
};
