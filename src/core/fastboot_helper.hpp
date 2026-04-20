#ifndef FASTBOOT_HELPER_HPP
#define FASTBOOT_HELPER_HPP

#include <string>
#include <functional>
#include <utility>
#include <vector>

class FastbootHelper {
public:
    using LogCallback = std::function<void(const std::string&)>;
    using ProgressCallback = std::function<void(const std::string&)>;

    FastbootHelper();
    ~FastbootHelper() = default;

    void set_log_callback(LogCallback callback);
    void set_progress_callback(ProgressCallback callback);

    bool flashing_unlock();
    bool erase_system();
    bool resize_logical_partition(const std::string& partition, const std::string& size);
    bool flash_system(const std::string& image_path);
    bool wipe_and_reboot();
    bool reboot();
    bool wait_for_device(int timeout_seconds = 30);
    bool is_device_connected();
    std::vector<std::string> list_devices();

    std::pair<int, std::string> execute_command(const std::string& cmd, bool silent = false);

private:
    LogCallback m_log_callback;
    ProgressCallback m_progress_callback;
};

#endif // FASTBOOT_HELPER_HPP
