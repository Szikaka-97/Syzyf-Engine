#pragma once

#include <AL/al.h>
#include <string>
#include <vector>

struct WavData
{
    ALenum format = 0;
    ALsizei sampleRate = 0;
    std::vector<char> data;
};

class WavLoader
{
public:
    static WavData Load(const std::string& path);
};