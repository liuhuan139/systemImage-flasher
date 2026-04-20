#include "adb_helper.hpp"
#include <array>
#include <cstdio>
#include <memory>
#include <thread>
#include <chrono>
#include <sstream>

AdbHelper::AdbHelper() = default;

void AdbHelper::set_log_callback(LogCallback callback) {
    m_log_callback = std::move(callback);
}

std::pair<int, std::string> AdbHelper::execute_command(const std::string& cmd, bool silent) {
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
        if (m_log_callback && !silent) {
            std::string line = buffer.data();
            if (!line.empty() && line.back() == '\n') {
                line.pop_back();
            }
            if (!line.empty()) {
                m_log_callback(line);
            }
        }
    }

    int exit_code = pclose(pipe);
    return {WEXITSTATUS(exit_code), result};
}

bool AdbHelper::reboot_bootloader() {
    auto [code, output] = execute_command("adb reboot bootloader");
    return code == 0;
}

bool AdbHelper::reboot_fastboot() {
    auto [code, output] = execute_command("adb reboot fastboot");
    return code == 0;
}

bool AdbHelper::reboot() {
    auto [code, output] = execute_command("adb reboot");
    return code == 0;
}

bool AdbHelper::wait_for_device(int timeout_seconds) {
    for (int i = 0; i < timeout_seconds; ++i) {
        if (is_device_connected()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return false;
}

std::string AdbHelper::get_device_state() {
    auto [code, output] = execute_command("adb get-state 2>&1", true);
    if (code == 0) {
        size_t pos = output.find_first_of("\r\n");
        if (pos != std::string::npos) {
            output = output.substr(0, pos);
        }
        return output;
    }
    return "";
}

bool AdbHelper::is_device_connected() {
    auto state = get_device_state();
    return state == "device" || state == "recovery";
}
