#ifndef FLASHER_HPP
#define FLASHER_HPP

#include <string>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <queue>
#include <sigc++/sigc++.h>
#include <glibmm/dispatcher.h>
#include "adb_helper.hpp"
#include "fastboot_helper.hpp"
#include "../utils/thread_worker.hpp"

enum class FlasherStatus {
    IDLE,
    WORKING,
    SUCCESS,
    ERROR,
    WAITING_USER,
    WARNING
};

struct StatusMessage {
    std::string text;
    FlasherStatus status;
};

enum class SignalType {
    NONE,
    STATUS,
    LOG,
    USER_ACTION,
    FINISHED,
    UNLOCK_COMPLETE_WAITING_REBOOT
};

class Flasher {
public:
    Flasher();
    ~Flasher();

    void select_image(const std::string& path);
    const std::string& get_image_path() const;

    void unlock_device();
    void confirm_unlock();
    void reboot_after_unlock();
    void flash_system();
    void cancel();

    bool is_busy() const;

    sigc::signal<void, const std::string&, FlasherStatus> signal_status_update();
    sigc::signal<void, const std::string&> signal_log_message();
    sigc::signal<void, const std::string&> signal_user_action_required();
    sigc::signal<void, bool> signal_finished();
    sigc::signal<void> signal_unlock_complete_waiting_reboot();
    sigc::signal<void, const std::string&> signal_progress();

private:
    void do_unlock_device();
    void do_reboot_after_unlock();
    void do_flash_system();
    void update_status(const std::string& message, FlasherStatus status);
    void log_message(const std::string& message);
    void emit_user_action_required(const std::string& message);
    void emit_finished(bool success);
    void emit_unlock_complete_waiting_reboot();
    void emit_progress(const std::string& message);

    void on_dispatcher_emit();

    std::string m_image_path;
    std::atomic<bool> m_busy;
    std::atomic<bool> m_cancelled;
    std::atomic<bool> m_unlock_confirmed;
    std::mutex m_wait_mutex;
    std::condition_variable m_wait_cv;

    std::unique_ptr<AdbHelper> m_adb;
    std::unique_ptr<FastbootHelper> m_fastboot;
    std::unique_ptr<ThreadWorker> m_worker;

    Glib::Dispatcher m_dispatcher;
    std::queue<StatusMessage> m_status_queue;
    std::queue<std::string> m_log_queue;
    std::queue<std::string> m_user_action_queue;
    std::queue<bool> m_finished_queue;
    std::queue<std::string> m_progress_queue;
    std::mutex m_queue_mutex;

    sigc::signal<void, const std::string&, FlasherStatus> m_signal_status_update;
    sigc::signal<void, const std::string&> m_signal_log_message;
    sigc::signal<void, const std::string&> m_signal_user_action_required;
    sigc::signal<void, bool> m_signal_finished;
    sigc::signal<void> m_signal_unlock_complete_waiting_reboot;
    sigc::signal<void, const std::string&> m_signal_progress;
};

#endif // FLASHER_HPP
