#pragma once

#include <fstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace Editor {

struct Settings {
    bool darkThemeEnabled = false;

    bool isMaximized = false;
    int windowWidth = 1280;
    int windowHeight = 720;

    void Load() {
        std::ifstream file("editor_settings.json");
        if (file.is_open()) {
            try {
                nlohmann::json j;
                file >> j;

                if (j.contains("darkThemeEnabled"))
                    this->darkThemeEnabled = j["darkThemeEnabled"].get<bool>();

                if (j.contains("windowWidth"))
                    this->windowWidth = j["windowWidth"].get<int>();

                if (j.contains("windowHeight"))
                    this->windowHeight = j["windowHeight"].get<int>();

                if (j.contains("isMaximized"))
                    this->isMaximized = j["isMaximized"].get<bool>();

            } catch (const nlohmann::json::parse_error& e) {
                spdlog::error("Editor: Settings file missing or corrupted, "
                              "using default values");
                this->darkThemeEnabled = false;
            }
        }
    }

    void Save() {
        std::ofstream file("editor_settings.json");
        if (file.is_open()) {
            nlohmann::json j;
            j["darkThemeEnabled"] = this->darkThemeEnabled;
            j["windowWidth"] = this->windowWidth;
            j["windowHeight"] = this->windowHeight;
            j["isMaximized"] = this->isMaximized;

            file << j.dump(4);
        }
    }
};
} // namespace Editor
