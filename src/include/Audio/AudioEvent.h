#pragma once
#include <string>
#include <Audio/AudioPlaybackSettings.h>
struct AudioEvent
{
    std::string soundName;
    AudioPlaybackSettings settings;
};