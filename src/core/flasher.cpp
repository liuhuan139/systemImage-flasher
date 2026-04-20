#include "flasher.hpp"
#include "../utils/logger.hpp"
#include <chrono>
#include <thread>
#include <atomic>

Flasher::Flasher()
    : m_busy(false)
    , m_cancelled(false)
    , m_unlock_confirmed(false)
    , m_adb(std::make_unique<AdbHelper>())
    , m_fastboot(std::make_unique<FastbootHelper>())
    , m_worker(std::make_unique<ThreadWorker>())
{
    m_dispatcher.connect(sigc::mem_fun(*this, &Flasher::on_dispatcher_emit));

    auto log_cb = [this](const std::string& msg) {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        m_log_queue.push(msg);
        m_dispatcher.emit();
    };
    m_adb->set_log_callback(log_cb);
    m_fastboot->set_log_callback(log_cb);

    auto progress_cb = [this](const std::string& msg) {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        m_progress_queue.push(msg);
        m_dispatcher.emit();
    };
    m_fastboot->set_progress_callback(progress_cb);
}

Flasher::~Flasher() {
    m_cancelled = true;
    m_unlock_confirmed = true;
    m_wait_cv.notify_all();
}

void Flasher::select_image(const std::string& path) {
    m_image_path = path;
    log_message("已选择镜像: " + path);
    update_status("镜像已选择", FlasherStatus::SUCCESS);
}

const std::string& Flasher::get_image_path() const {
    return m_image_path;
}

void Flasher::unlock_device() {
    if (m_busy) {
        log_message("错误: 操作正在进行中");
        return;
    }
    m_busy = true;
    m_cancelled = false;
    m_unlock_confirmed = false;

    m_worker->run_async([this]() {
        do_unlock_device();
    }, [this](bool success, const std::string& error) {
        if (!success && !error.empty()) {
            log_message("错误: " + error);
        }
    });
}

void Flasher::confirm_unlock() {
    m_unlock_confirmed = true;
    m_wait_cv.notify_all();
}

void Flasher::reboot_after_unlock() {
    if (m_busy) {
        log_message("错误: 操作正在进行中");
        return;
    }
    m_busy = true;
    m_cancelled = false;

    m_worker->run_async([this]() {
        do_reboot_after_unlock();
    }, [this](bool success, const std::string& error) {
        m_busy = false;
        if (!success && !error.empty()) {
            log_message("错误: " + error);
        }
    });
}

void Flasher::do_unlock_device() {
    update_status("步骤 1/4: 检查设备连接...", FlasherStatus::WORKING);
    if (!m_adb->is_device_connected()) {
        update_status("错误: 未检测到 ADB 设备", FlasherStatus::ERROR);
        m_busy = false;
        emit_finished(false);
        return;
    }

    update_status("步骤 2/4: 正在重启到 Bootloader 模式...", FlasherStatus::WORKING);
    if (!m_adb->reboot_bootloader()) {
        update_status("警告: 重启命令发送失败，请手动进入 Bootloader", FlasherStatus::WARNING);
    }

    update_status("步骤 3/4: 等待设备进入 Fastboot 模式...", FlasherStatus::WORKING);
    if (!m_fastboot->wait_for_device(60)) {
        update_status("错误: 未检测到 Fastboot 设备", FlasherStatus::ERROR);
        m_busy = false;
        emit_finished(false);
        return;
    }

    update_status("步骤 4/4: 设备已进入 Fastboot 模式", FlasherStatus::WAITING_USER);
    emit_user_action_required("设备已进入 Fastboot 模式！\n\n请点击\"确认解锁\"按钮发送解锁命令，\n然后立即在设备上按音量上键确认。");

    update_status("等待用户点击确认以发送解锁命令...", FlasherStatus::WAITING_USER);
    {
        std::unique_lock<std::mutex> lock(m_wait_mutex);
        m_wait_cv.wait(lock, [this]() {
            return m_unlock_confirmed || m_cancelled;
        });
    }

    if (m_cancelled) {
        update_status("操作已取消", FlasherStatus::ERROR);
        m_busy = false;
        emit_finished(false);
        return;
    }

    update_status("正在执行 fastboot flashing unlock...", FlasherStatus::WORKING);
    m_fastboot->flashing_unlock();

    update_status("请在设备上按音量键确认解锁（5秒内）", FlasherStatus::WAITING_USER);
    log_message("提示：请在设备屏幕上按音量上键确认解锁！");

    update_status("等待设备解锁...", FlasherStatus::WAITING_USER);
    std::this_thread::sleep_for(std::chrono::seconds(8));

    update_status("设备解锁完成！", FlasherStatus::SUCCESS);
    m_busy = false;
    emit_unlock_complete_waiting_reboot();
}

void Flasher::do_reboot_after_unlock() {
    update_status("正在执行 fastboot reboot...", FlasherStatus::WORKING);
    m_fastboot->reboot();

    update_status("等待设备重启并连接...", FlasherStatus::WORKING);
    if (!m_adb->wait_for_device(120)) {
        update_status("错误: 等待设备连接超时", FlasherStatus::ERROR);
        emit_finished(false);
        return;
    }

    update_status("设备已连接！现在可以开始烧录 System 镜像", FlasherStatus::SUCCESS);
    emit_finished(true);
}

void Flasher::flash_system() {
    if (m_busy) {
        log_message("错误: 操作正在进行中");
        return;
    }
    if (m_image_path.empty()) {
        update_status("错误: 请先选择 System.img 镜像文件", FlasherStatus::ERROR);
        return;
    }

    m_busy = true;
    m_cancelled = false;

    m_worker->run_async([this]() {
        do_flash_system();
    }, [this](bool success, const std::string& error) {
        m_busy = false;
        if (!success && !error.empty()) {
            log_message("错误: " + error);
        }
    });
}

void Flasher::do_flash_system() {
    update_status("步骤 1/8: 检查设备连接...", FlasherStatus::WORKING);
    if (!m_adb->is_device_connected()) {
        update_status("错误: 未检测到 ADB 设备，请先确保设备已连接", FlasherStatus::ERROR);
        emit_finished(false);
        return;
    }

    update_status("步骤 2/8: 正在重启到 Fastboot 模式...", FlasherStatus::WORKING);
    m_adb->reboot_fastboot();

    update_status("步骤 3/8: 等待 Fastboot 设备...", FlasherStatus::WORKING);
    if (!m_fastboot->wait_for_device(60)) {
        update_status("错误: 未检测到 Fastboot 设备", FlasherStatus::ERROR);
        emit_finished(false);
        return;
    }

    update_status("步骤 4/8: 正在擦除 system 分区...", FlasherStatus::WORKING);
    if (m_cancelled) {
        update_status("操作已取消", FlasherStatus::ERROR);
        emit_finished(false);
        return;
    }
    if (!m_fastboot->erase_system()) {
        log_message("警告: 擦除 system 分区可能失败，继续尝试...");
    }

    update_status("步骤 5/8: 正在调整分区大小...", FlasherStatus::WORKING);
    std::vector<std::string> partitions = {"product_a", "product_b", "system_ext_a", "system_ext_b"};
    for (const auto& part : partitions) {
        if (m_cancelled) {
            update_status("操作已取消", FlasherStatus::ERROR);
            emit_finished(false);
            return;
        }
        log_message("调整分区: " + part);
        if (!m_fastboot->resize_logical_partition(part, "100")) {
            log_message("警告: 调整分区 " + part + " 可能失败，继续尝试...");
        }
    }

    update_status("步骤 6/8: 正在烧录 System 镜像 (这可能需要几分钟)...", FlasherStatus::WORKING);
    if (m_cancelled) {
        update_status("操作已取消", FlasherStatus::ERROR);
        emit_finished(false);
        return;
    }
    if (!m_fastboot->flash_system(m_image_path)) {
        update_status("错误: 烧录镜像失败", FlasherStatus::ERROR);
        emit_finished(false);
        return;
    }

    update_status("步骤 7/8: 烧录成功！", FlasherStatus::SUCCESS);
    update_status("步骤 8/8: 正在擦除数据并重启...", FlasherStatus::WORKING);
    if (!m_fastboot->wipe_and_reboot()) {
        update_status("警告: 重启命令发送失败", FlasherStatus::WARNING);
    }

    update_status("完成！", FlasherStatus::SUCCESS);
    update_status("设备正在重启，首次启动可能需要较长时间", FlasherStatus::SUCCESS);
    emit_finished(true);
}

void Flasher::cancel() {
    m_cancelled = true;
    m_wait_cv.notify_all();
}

bool Flasher::is_busy() const {
    return m_busy;
}

void Flasher::update_status(const std::string& message, FlasherStatus status) {
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    m_status_queue.push({message, status});
    m_dispatcher.emit();
}

void Flasher::log_message(const std::string& message) {
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    m_log_queue.push(message);
    m_dispatcher.emit();
}

void Flasher::emit_user_action_required(const std::string& message) {
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    m_user_action_queue.push(message);
    m_dispatcher.emit();
}

void Flasher::emit_finished(bool success) {
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    m_finished_queue.push(success);
    m_dispatcher.emit();
}

void Flasher::emit_unlock_complete_waiting_reboot() {
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    m_status_queue.push({"解锁完成，请点击\"重启设备\"按钮", FlasherStatus::SUCCESS});
    m_dispatcher.emit();

    m_signal_unlock_complete_waiting_reboot.emit();
}

void Flasher::emit_progress(const std::string& message) {
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    m_progress_queue.push(message);
    m_dispatcher.emit();
}

void Flasher::on_dispatcher_emit() {
    std::queue<StatusMessage> status_queue;
    std::queue<std::string> log_queue;
    std::queue<std::string> user_action_queue;
    std::queue<bool> finished_queue;
    std::queue<std::string> progress_queue;

    {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        status_queue.swap(m_status_queue);
        log_queue.swap(m_log_queue);
        user_action_queue.swap(m_user_action_queue);
        finished_queue.swap(m_finished_queue);
        progress_queue.swap(m_progress_queue);
    }

    while (!status_queue.empty()) {
        auto& msg = status_queue.front();
        m_signal_status_update.emit(msg.text, msg.status);
        status_queue.pop();
    }

    while (!log_queue.empty()) {
        m_signal_log_message.emit(log_queue.front());
        log_queue.pop();
    }

    while (!user_action_queue.empty()) {
        m_signal_user_action_required.emit(user_action_queue.front());
        user_action_queue.pop();
    }

    while (!finished_queue.empty()) {
        m_signal_finished.emit(finished_queue.front());
        finished_queue.pop();
    }

    while (!progress_queue.empty()) {
        m_signal_progress.emit(progress_queue.front());
        progress_queue.pop();
    }
}

sigc::signal<void, const std::string&, FlasherStatus> Flasher::signal_status_update() {
    return m_signal_status_update;
}

sigc::signal<void, const std::string&> Flasher::signal_log_message() {
    return m_signal_log_message;
}

sigc::signal<void, const std::string&> Flasher::signal_user_action_required() {
    return m_signal_user_action_required;
}

sigc::signal<void, bool> Flasher::signal_finished() {
    return m_signal_finished;
}

sigc::signal<void> Flasher::signal_unlock_complete_waiting_reboot() {
    return m_signal_unlock_complete_waiting_reboot;
}

sigc::signal<void, const std::string&> Flasher::signal_progress() {
    return m_signal_progress;
}
