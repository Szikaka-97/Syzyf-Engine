#include <WavLoader.h>
#include <fstream>
#include <stdexcept>
#include <cstdint>

namespace
{
    template<typename T>
    void Read(std::ifstream& file, T& value)
    {
        file.read(reinterpret_cast<char*>(&value), sizeof(T));

        if (!file)
            throw std::runtime_error("Failed to read WAV file");
    }
}

WavData WavLoader::Load(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("Failed to open WAV file: " + path);

    char riff[4];
    char wave[4];
    char chunkId[4];
    uint32_t chunkSize = 0;
    uint16_t audioFormat = 0;
    uint16_t channels = 0;
    uint32_t sampleRate = 0;
    uint32_t byteRate = 0;
    uint16_t blockAlign = 0;
    uint16_t bitsPerSample = 0;

    file.read(riff, 4);
    Read(file, chunkSize);
    file.read(wave, 4);

    if (std::string(riff, 4) != "RIFF" || std::string(wave, 4) != "WAVE")
        throw std::runtime_error("Invalid WAV file header: " + path);

    bool foundFmt = false;
    bool foundData = false;

    WavData result;

    while (file && (!foundFmt || !foundData))
    {
        file.read(chunkId, 4);
        if (!file)
            break;

        Read(file, chunkSize);

        std::string id(chunkId, 4);

        if (id == "fmt ")
        {
            Read(file, audioFormat);
            Read(file, channels);
            Read(file, sampleRate);
            Read(file, byteRate);
            Read(file, blockAlign);
            Read(file, bitsPerSample);

            if (chunkSize > 16)
                file.seekg(chunkSize - 16, std::ios::cur);

            foundFmt = true;
        }
        else if (id == "data")
        {
            result.data.resize(chunkSize);
            file.read(result.data.data(), chunkSize);

            if (!file)
                throw std::runtime_error("Failed to read WAV data: " + path);

            foundData = true;
        }
        else
        {
            file.seekg(chunkSize, std::ios::cur);
        }
    }

    if (!foundFmt)
        throw std::runtime_error("WAV file missing fmt chunk: " + path);

    if (!foundData)
        throw std::runtime_error("WAV file missing data chunk: " + path);

    if (audioFormat != 1)
        throw std::runtime_error("Only PCM WAV is supported: " + path);

    if (channels == 1 && bitsPerSample == 8)
        result.format = AL_FORMAT_MONO8;
    else if (channels == 1 && bitsPerSample == 16)
        result.format = AL_FORMAT_MONO16;
    else if (channels == 2 && bitsPerSample == 8)
        result.format = AL_FORMAT_STEREO8;
    else if (channels == 2 && bitsPerSample == 16)
        result.format = AL_FORMAT_STEREO16;
    else
        throw std::runtime_error("Unsupported WAV format: " + path);

    result.sampleRate = static_cast<ALsizei>(sampleRate);

    return result;
}