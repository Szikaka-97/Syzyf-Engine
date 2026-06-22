#pragma once

#include <fstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

struct GameSettings {
    int resolutionWidth = 1280;
    int resolutionHeight = 720;
    bool windowed = true;
    bool vsyncEnabled = true;

    bool ssaoEnabled = true;
    float soundVolume = 1.0f;
    float ambientBrightness = 1.0f;

    void Load() {
        std::ifstream file("game_settings.json");
        if (file.is_open()) {
            try {
                nlohmann::json j;
                file >> j;

                if (j.contains("resolutionWidth"))
                    this->resolutionWidth = j["resolutionWidth"].get<int>();

                if (j.contains("resolutionHeight"))
                    this->resolutionHeight = j["resolutionHeight"].get<int>();

                if (j.contains("windowed"))
                    this->windowed = j["windowed"].get<bool>();

                if (j.contains("vsyncEnabled"))
                    this->vsyncEnabled = j["vsyncEnabled"].get<bool>();

                if (j.contains("ssaoEnabled"))
                    this->ssaoEnabled = j["ssaoEnabled"].get<bool>();

                if (j.contains("soundVolume"))
                    this->soundVolume = j["soundVolume"].get<float>();

                if (j.contains("ambientBrightness"))
                    this->ambientBrightness = j["ambientBrightness"].get<float>();

            } catch (const nlohmann::json::parse_error& e) {
                spdlog::error(
                    "Game settings file missing or corrupted, using defaults");
            }
        }
    }

    void Save() {
        std::ofstream file("game_settings.json");
        if (file.is_open()) {
            nlohmann::json j;
            j["resolutionWidth"] = this->resolutionWidth;
            j["resolutionHeight"] = this->resolutionHeight;
            j["windowed"] = this->windowed;
            j["vsyncEnabled"] = this->vsyncEnabled;
            j["ssaoEnabled"] = this->ssaoEnabled;
            j["soundVolume"] = this->soundVolume;
            j["ambientBrightness"] = this->ambientBrightness;

            file << j.dump(4);
        }
    }
};
