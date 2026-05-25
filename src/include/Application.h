#pragma once

#include <SDL3/SDL_video.h>

#include <string>

class Scene;

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

public:
    Application(const std::string& title = "Syzyf", int width = 1280, int height = 720);
    virtual ~Application();

    void Run();

    static SDL_Window* GetWindow();
    static Scene* GetCurrentScene();

protected:
    virtual void OnInit() {}
    virtual void OnUpdate() {}
    virtual void OnRender() {}
    virtual void OnImGuiRender() {}
    virtual void OnShutdown() {}

private:
    bool InitEngine();
    void ShutdownEngine();
};
