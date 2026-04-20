#include "fastboot_helper.hpp"
#include <array>
#include <cstdio>
#include <memory>
#include <thread>
#include <chrono>
#include <sstream>

FastbootHelper::FastbootHelper() = default;

void FastbootHelper::set_log_callback(LogCallback callback) {
    m_log_callback = std::move(callback);
}

void FastbootHelper::set_progress_callback(ProgressCallback callback) {
    m_progress_callback = std::move(callback);
}

std::pair<int, std::string> FastbootHelper::execute_command(const std::string& cmd, bool silent) {
    std::array<char, 128> buffer;
    std::string result;

    if (m_log_callback && !silent) {
        m_log_callback("$ " + cmd);
    }

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        return {-1, "Failed to open pipe"};
    }

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
        std::string line = buffer.data();
        if (!line.empty() && line.back() == '\n') {
            line.pop_back();
        }
        if (!line.empty()) {
            if (m_progress_callback) {
                m_progress_callback(line);
            }
            if (m_log_callback && !silent) {
                m_log_callback(line);
            }
        }
    }

    int exit_code = pclose(pipe);
    return {WEXITSTATUS(exit_code), result};
}

bool FastbootHelper::flashing_unlock() {
    auto [code, output] = execute_command("fastboot flashing unlock");
    return code == 0;
}

bool FastbootHelper::erase_system() {
    auto [code, output] = execute_command("fastboot erase system");
    return code == 0;
}

bool FastbootHelper::resize_logical_partition(const std::string& partition, const std::string& size) {
    auto [code, output] = execute_command("fastboot resize-logical-partition " + partition + " " + size);
    return code == 0;
}

bool FastbootHelper::flash_system(const std::string& image_path) {
    auto [code, output] = execute_command("fastboot flash system \"" + image_path + "\"");
    return code == 0;
}

bool FastbootHelper::wipe_and_reboot() {
    auto [code, output] = execute_command("fastboot -w reboot");
    return code == 0;
}

bool FastbootHelper::reboot() {
    auto [code, output] = execute_command("fastboot reboot");
    return code == 0;
}

bool FastbootHelper::wait_for_device(int timeout_seconds) {
    for (int i = 0; i < timeout_seconds; ++i) {
        if (is_device_connected()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return false;
}

bool FastbootHelper::is_device_connected() {
    auto devices = list_devices();
    return !devices.empty();
}

std::vector<std::string> FastbootHelper::list_devices() {
    std::vector<std::string> devices;
    auto [code, output] = execute_command("fastboot devices", true);
    if (code == 0) {
        std::istringstream iss(output);
        std::string line;
        while (std::getline(iss, line)) {
            if (!line.empty()) {
                devices.push_back(line);
            }
        }
    }
    return devices;
}
