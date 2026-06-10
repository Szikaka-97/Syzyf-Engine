#include "include/GameApplication.h"

#include "imgui.h"
#include "scenes/CraftingScene.h"
#include "scenes/SplashScene.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

void GameApplication::OnInit(int argc, char* argv[]) {
    auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto logger = std::make_shared<spdlog::logger>("Bimberman", sink);
    spdlog::set_default_logger(logger);

    spdlog::level::level_enum logLevel = spdlog::level::err;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "--log-level" || arg == "-l") && i + 1 < argc) {
            std::string levelString = argv[++i];

            if (levelString == "trace")
                logLevel = spdlog::level::trace;
            else if (levelString == "debug")
                logLevel = spdlog::level::debug;
            else if (levelString == "info")
                logLevel = spdlog::level::info;
            else if (levelString == "warn")
                logLevel = spdlog::level::warn;
            else if (levelString == "err")
                logLevel = spdlog::level::err;
            else if (levelString == "critical")
                logLevel = spdlog::level::critical;
            else if (levelString == "off")
                logLevel = spdlog::level::off;
            else
                logLevel = spdlog::level::err;

            break;
        }
    }

    spdlog::set_level(logLevel);

    this->settings.Load();

    Scene* newScene = Scene::CreateStandaloneScene();
    SplashScene::InitScene(*newScene);
    this->currentScene = newScene;
    this->ApplySettings();
}

void GameApplication::ApplySettings() {
    SDL_SetWindowSize(this->window, this->settings.resolutionWidth,
                      this->settings.resolutionHeight);

    if (this->settings.windowed) {
        SDL_SetWindowFullscreen(this->window, 0);
    } else {
        SDL_SetWindowFullscreen(this->window, SDL_WINDOW_FULLSCREEN);
    }

    if (this->currentScene) {
        if (this->currentScene->GetGraphics()) {
            this->currentScene->GetGraphics()->SetSSAOEnabled(
                this->settings.ssaoEnabled);
        }
    }
}

void GameApplication::OnUpdate() {
    if (this->currentScene) {
        this->currentScene->Update();
        this->currentScene->FlushQueues();
    }
}

void GameApplication::OnRender() {
    if (this->currentScene) {
        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        this->currentScene->GetGraphics()->UpdateScreenResolution(
            glm::vec2(w, h));
        this->currentScene->Render();
    }
}

void GameApplication::OnImGuiRender() {
    if (ImGui::Shortcut(ImGuiKey_GraveAccent, ImGuiInputFlags_RouteGlobal)) {
        this->displayDebug = !this->displayDebug;
    }

    if (this->displayDebug && this->currentScene != nullptr) {
        this->currentScene->DrawImGui();
    }
}
