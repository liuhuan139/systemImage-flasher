#include "logger.hpp"

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

void Logger::set_callback(LogCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_callback = std::move(callback);
}

void Logger::log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_callback) {
        m_callback(level, message);
    }
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warning(const std::string& message) {
    log(LogLevel::WARNING, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void Logger::success(const std::string& message) {
    log(LogLevel::SUCCESS, message);
}
