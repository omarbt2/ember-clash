#pragma once

#include <string_view>

namespace ember {

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error,
};

class Logger {
public:
    using Level = LogLevel;

    static void set_min_level(Level level);
    static void log(Level level, std::string_view message);

    static void debug(std::string_view msg) { log(Level::Debug, msg); }
    static void info (std::string_view msg) { log(Level::Info,  msg); }
    static void warn (std::string_view msg) { log(Level::Warning, msg); }
    static void error(std::string_view msg) { log(Level::Error, msg); }
};

} // namespace ember