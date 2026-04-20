#ifndef ADB_HELPER_HPP
#define ADB_HELPER_HPP

#include <string>
#include <functional>
#include <utility>

class AdbHelper {
public:
    using LogCallback = std::function<void(const std::string&)>;

    AdbHelper();
    ~AdbHelper() = default;

    void set_log_callback(LogCallback callback);

    bool reboot_bootloader();
    bool reboot_fastboot();
    bool reboot();
    bool wait_for_device(int timeout_seconds = 30);
    std::string get_device_state();
    bool is_device_connected();

    std::pair<int, std::string> execute_command(const std::string& cmd, bool silent = false);

private:
    LogCallback m_log_callback;
};

#endif // ADB_HELPER_HPP
