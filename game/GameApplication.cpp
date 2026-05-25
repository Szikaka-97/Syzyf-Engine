#include "include/GameApplication.h"

#include "TestScene.h"
#include "imgui.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

void GameApplication::OnInit() {
    auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto logger = std::make_shared<spdlog::logger>("Bimberman", sink);
    spdlog::set_default_logger(logger);
    spdlog::set_level(spdlog::level::err);

    this->currentScene = Scene::CreateStandaloneScene();
    TestScene::InitScene(*this->currentScene);
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
