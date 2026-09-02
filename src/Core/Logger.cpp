#include "Core/Logger.h"

#include <atomic>
#include <cstdio>
#include <mutex>
#include <string>

namespace ember {

namespace {

std::atomic<LogLevel> g_min_level{LogLevel::Info};
std::mutex            g_io_mutex;

const char* level_prefix(LogLevel level) {
    switch (level) {
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO ";
        case LogLevel::Warning: return "WARN ";
        case LogLevel::Error:   return "ERROR";
    }
    return "?????";
}

} // namespace

void Logger::set_min_level(Level level) {
    g_min_level.store(level, std::memory_order_relaxed);
}

void Logger::log(Level level, std::string_view message) {
    if (static_cast<int>(level) < static_cast<int>(g_min_level.load(std::memory_order_relaxed))) {
        return;
    }

    // Serialize multi-threaded writes to stdio; SDL3 starts a render thread
    // for some backends, and we want lines to stay intact.
    std::lock_guard<std::mutex> lock(g_io_mutex);

    std::fprintf(level == LogLevel::Error ? stderr : stdout,
                 "[%s] %.*s\n",
                 level_prefix(level),
                 static_cast<int>(message.size()),
                 message.data());
    std::fflush(level == LogLevel::Error ? stderr : stdout);
}

} // namespace ember