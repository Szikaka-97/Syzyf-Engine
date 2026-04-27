#pragma once

#include "Themes.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace Editor {

struct Settings {
  private:
    static const Themes::Theme DEFAULT_THEME = Themes::Theme::PurpleDark;
    static const bool DEFAULT_IS_MAXIMIZED = false;
    static const int DEFAULT_WINDOW_WIDTH = 1280;
    static const int DEFAULT_WINDOW_HEIGHT = 720;

  public:
    Themes::Theme theme = DEFAULT_THEME;
    bool isMaximized = DEFAULT_IS_MAXIMIZED;
    int windowWidth = DEFAULT_WINDOW_WIDTH;
    int windowHeight = DEFAULT_WINDOW_HEIGHT;

    void Load() {
        std::ifstream file("editor_settings.json");
        if (file.is_open()) {
            try {
                nlohmann::json j;
                file >> j;

                if (j.contains("theme"))
                    this->theme = j["theme"].get<Themes::Theme>();

                if (j.contains("windowWidth"))
                    this->windowWidth = j["windowWidth"].get<int>();

                if (j.contains("windowHeight"))
                    this->windowHeight = j["windowHeight"].get<int>();

                if (j.contains("isMaximized"))
                    this->isMaximized = j["isMaximized"].get<bool>();

            } catch (const nlohmann::json::parse_error& e) {
                spdlog::error("Editor: Settings file missing or corrupted, "
                              "using default values");
                Themes::Theme theme = Settings::DEFAULT_THEME;
                bool isMaximized = Settings::DEFAULT_IS_MAXIMIZED;
                int windowWidth = Settings::DEFAULT_WINDOW_WIDTH;
                int windowHeight = Settings::DEFAULT_WINDOW_HEIGHT;
            }
        }
    }

    void Save() {
        std::ofstream file("editor_settings.json");
        if (file.is_open()) {
            nlohmann::json j;
            j["theme"] = this->theme;
            j["windowWidth"] = this->windowWidth;
            j["windowHeight"] = this->windowHeight;
            j["isMaximized"] = this->isMaximized;

            file << j.dump(4);
        }
    }
};
} // namespace Editor

NLOHMANN_JSON_SERIALIZE_ENUM(Editor::Themes::Theme,
                             {{Editor::Themes::Theme::Dark, "Dark"},
                              {Editor::Themes::Theme::Light, "Light"},
                              {Editor::Themes::Theme::Classic, "Classic"},
                              {Editor::Themes::Theme::PurpleDark, "PurpleDark"},
                              {Editor::Themes::Theme::PurpleLight,
                               "PurpleLight"}})
