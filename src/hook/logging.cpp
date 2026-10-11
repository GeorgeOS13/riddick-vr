#include "logging.h"
#include <windows.h>
#include <cstdarg>
#include <cstdio>
#include <ctime>

namespace RiddickVR {

Logger& Logger::Instance() {
    static Logger s_instance;
    return s_instance;
}

Logger::~Logger() {
    if (m_file.is_open()) {
        m_file.close();
    }
}

void Logger::Initialize(const std::string& filename) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_initialized) return;

    m_file.open(filename, std::ios::out | std::ios::trunc);
    m_initialized = m_file.is_open();
    if (m_initialized) {
        char timeStr[64];
        time_t now = time(nullptr);
        tm localTime;
        localtime_s(&localTime, &now);
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &localTime);
        m_file << "=== Riddick VR Log Started at " << timeStr << " ===" << std::endl;
        m_file.flush();
    }
}

void Logger::Log(LogLevel level, const char* format, ...) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized || !m_file.is_open()) return;

    const char* levelStr = "INFO";
    switch (level) {
        case LogLevel::Debug:   levelStr = "DEBUG"; break;
        case LogLevel::Info:    levelStr = "INFO";  break;
        case LogLevel::Warning: levelStr = "WARN";  break;
        case LogLevel::Error:   levelStr = "ERROR"; break;
    }

    char buffer[2048];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    m_file << "[" << levelStr << "] " << buffer << std::endl;
    m_file.flush();

    // Also output to Windows debugger console
    OutputDebugStringA(buffer);
    OutputDebugStringA("\n");
}

} // namespace RiddickVR
