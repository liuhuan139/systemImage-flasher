#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include <gtkmm.h>
#include "status_bar.hpp"
#include "../core/flasher.hpp"
#include "../utils/logger.hpp"
#include "../core/adb_helper.hpp"
#include "../core/fastboot_helper.hpp"

class MainWindow : public Gtk::Window {
public:
    MainWindow();
    ~MainWindow() = default;

private:
    void setup_menu();
    void setup_ui();
    void connect_signals();

    void on_file_choose();
    void on_unlock_clicked();
    void on_flash_clicked();
    void on_detect_clicked();
    void on_confirm_unlock_clicked();
    void on_reboot_after_unlock_clicked();
    void on_reboot_device_clicked();
    void on_unlock_complete_waiting_reboot();
    void on_toggle_log_clicked();
    void on_show_instructions_clicked();

    void on_status_update(const std::string& message, FlasherStatus status);
    void on_log_message(const std::string& message);
    void on_user_action_required(const std::string& message);
    void on_finished(bool success);
    void on_logger_message(LogLevel level, const std::string& message);
    void on_progress(const std::string& message);

    void append_log(const std::string& text, const std::string& color = "");

    void show_about_dialog();

    Gtk::VBox* m_main_vbox;
    Gtk::MenuBar* m_menu_bar;
    Gtk::FileChooserButton* m_file_chooser;
    Gtk::Button* m_btn_detect;
    Gtk::Button* m_btn_unlock;
    Gtk::Button* m_btn_confirm_unlock;
    Gtk::Button* m_btn_reboot_after_unlock;
    Gtk::Button* m_btn_reboot_device;
    Gtk::Button* m_btn_flash;
    Gtk::Button* m_btn_toggle_log;
    Gtk::TextView* m_log_view;
    Gtk::Frame* m_log_frame;
    Gtk::Label* m_instructions_label;
    StatusBar* m_status_bar;

    Glib::RefPtr<Gtk::TextBuffer> m_log_buffer;

    std::unique_ptr<Flasher> m_flasher;
    std::unique_ptr<AdbHelper> m_adb;
    std::unique_ptr<FastbootHelper> m_fastboot;

    bool m_log_visible;
};

#endif // MAIN_WINDOW_HPP
