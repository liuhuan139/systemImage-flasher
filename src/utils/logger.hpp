#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <functional>
#include <mutex>

enum class LogLevel {
    INFO,
    WARNING,
    ERROR,
    SUCCESS
};

class Logger {
public:
    using LogCallback = std::function<void(LogLevel, const std::string&)>;

    static Logger& instance();

    void set_callback(LogCallback callback);
    void log(LogLevel level, const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    void success(const std::string& message);

private:
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    LogCallback m_callback;
    std::mutex m_mutex;
};

#endif // LOGGER_HPP
