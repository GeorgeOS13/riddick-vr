#pragma once

#include <string>
#include <fstream>
#include <mutex>

namespace RiddickVR {

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

class Logger {
public:
    static Logger& Instance();

    void Initialize(const std::string& filename = "riddick_vr.log");
    void Log(LogLevel level, const char* format, ...);

private:
    Logger() = default;
    ~Logger();

    std::ofstream m_file;
    std::mutex m_mutex;
    bool m_initialized = false;
};

#define VR_LOG_DEBUG(fmt, ...) RiddickVR::Logger::Instance().Log(RiddickVR::LogLevel::Debug, fmt, ##__VA_ARGS__)
#define VR_LOG_INFO(fmt, ...)  RiddickVR::Logger::Instance().Log(RiddickVR::LogLevel::Info, fmt, ##__VA_ARGS__)
#define VR_LOG_WARN(fmt, ...)  RiddickVR::Logger::Instance().Log(RiddickVR::LogLevel::Warning, fmt, ##__VA_ARGS__)
#define VR_LOG_ERROR(fmt, ...) RiddickVR::Logger::Instance().Log(RiddickVR::LogLevel::Error, fmt, ##__VA_ARGS__)

} // namespace RiddickVR
