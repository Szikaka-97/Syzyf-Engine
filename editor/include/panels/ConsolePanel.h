#pragma once

#include <deque>
#include <imgui.h>
#include <spdlog/sinks/base_sink.h>
#include <string>

namespace Editor {
template <typename Mutex>
class ImGuiConsoleSink : public spdlog::sinks::base_sink<Mutex> {
  public:
    inline static std::deque<std::string> logs;
    inline static const std::size_t MAX_LOGS = 500;
    inline static bool autoScroll;

  protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        spdlog::memory_buf_t formatted;
        spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);

        logs.push_back(fmt::to_string(formatted));
        if (logs.size() > MAX_LOGS) {
            logs.pop_front();
        }
    }

    void flush_() override {}
};

class ConsolePanel {
  public:
    void Draw();
};
} // namespace Editor
