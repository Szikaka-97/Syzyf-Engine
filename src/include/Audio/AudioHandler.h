#pragma once

#include <cstdint>

class AudioHandler
{
public:
    AudioHandler() = default;
    explicit AudioHandler(uint32_t id) : m_id(id) {}

    bool IsValid() const { return m_id != 0; }
    uint32_t GetId() const { return m_id; }

private:
    uint32_t m_id = 0;
};