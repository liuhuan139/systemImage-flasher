#include "main_window.hpp"
#include <sstream>

MainWindow::MainWindow()
    : m_flasher(std::make_unique<Flasher>())
    , m_adb(std::make_unique<AdbHelper>())
    , m_fastboot(std::make_unique<FastbootHelper>())
    , m_log_visible(true)
{
    set_title("System Img Flasher");
    set_default_size(700, 500);
    set_border_width(10);

    setup_ui();
    connect_signals();

    Logger::instance().set_callback(
        sigc::mem_fun(*this, &MainWindow::on_logger_message));
}

void MainWindow::setup_menu() {
    m_menu_bar = Gtk::manage(new Gtk::MenuBar());

    auto file_menu = Gtk::manage(new Gtk::Menu());
    auto file_item = Gtk::manage(new Gtk::MenuItem("_文件", true));
    file_item->set_submenu(*file_menu);

    auto choose_image_item = Gtk::manage(new Gtk::MenuItem("选择镜像..."));
    choose_image_item->signal_activate().connect([this]() {
        Gtk::FileChooserDialog dialog(*this, "选择 System.img", Gtk::FILE_CHOOSER_ACTION_OPEN);
        dialog.add_button("_取消", Gtk::RESPONSE_CANCEL);
        dialog.add_button("_选择", Gtk::RESPONSE_OK);

        auto filter_img = Gtk::FileFilter::create();
        filter_img->set_name("镜像文件 (*.img)");
        filter_img->add_pattern("*.img");
        dialog.add_filter(filter_img);

        auto filter_any = Gtk::FileFilter::create();
        filter_any->set_name("所有文件");
        filter_any->add_pattern("*");
        dialog.add_filter(filter_any);

        if (dialog.run() == Gtk::RESPONSE_OK) {
            m_file_chooser->set_filename(dialog.get_filename());
            on_file_choose();
        }
    });
    file_menu->append(*choose_image_item);

    file_menu->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));

    auto quit_item = Gtk::manage(new Gtk::MenuItem("退出"));
    quit_item->signal_activate().connect([this]() { close(); });
    file_menu->append(*quit_item);

    m_menu_bar->append(*file_item);

    auto tools_menu = Gtk::manage(new Gtk::Menu());
    auto tools_item = Gtk::manage(new Gtk::MenuItem("_工具", true));
    tools_item->set_submenu(*tools_menu);

    auto detect_item = Gtk::manage(new Gtk::MenuItem("检测设备"));
    detect_item->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_detect_clicked));
    tools_menu->append(*detect_item);

    auto reboot_item = Gtk::manage(new Gtk::MenuItem("重启设备"));
    reboot_item->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_reboot_device_clicked));
    tools_menu->append(*reboot_item);

    m_menu_bar->append(*tools_item);

    auto help_menu = Gtk::manage(new Gtk::Menu());
    auto help_item = Gtk::manage(new Gtk::MenuItem("_帮助", true));
    help_item->set_submenu(*help_menu);

    auto instructions_item = Gtk::manage(new Gtk::MenuItem("操作说明"));
    instructions_item->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_show_instructions_clicked));
    help_menu->append(*instructions_item);

    help_menu->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));

    auto about_item = Gtk::manage(new Gtk::MenuItem("关于"));
    about_item->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::show_about_dialog));
    help_menu->append(*about_item);

    m_menu_bar->append(*help_item);

    m_main_vbox->pack_start(*m_menu_bar, false, false, 0);
}

void MainWindow::setup_ui() {
    m_main_vbox = Gtk::manage(new Gtk::VBox(false, 8));
    add(*m_main_vbox);

    auto center_grid = Gtk::manage(new Gtk::Grid());
    center_grid->set_row_spacing(10);
    center_grid->set_column_spacing(10);
    center_grid->set_border_width(5);

    center_grid->attach(*Gtk::manage(new Gtk::Label("镜像文件:")), 0, 0, 1, 1);

    m_file_chooser = Gtk::manage(new Gtk::FileChooserButton("选择 System.img", Gtk::FILE_CHOOSER_ACTION_OPEN));
    auto filter_img = Gtk::FileFilter::create();
    filter_img->set_name("镜像文件 (*.img)");
    filter_img->add_pattern("*.img");
    m_file_chooser->add_filter(filter_img);
    m_file_chooser->set_hexpand(true);
    center_grid->attach(*m_file_chooser, 1, 0, 3, 1);

    auto button_box = Gtk::manage(new Gtk::HBox(false, 8));

    m_btn_detect = Gtk::manage(new Gtk::Button("检测设备"));
    button_box->pack_start(*m_btn_detect, false, false, 5);

    m_btn_unlock = Gtk::manage(new Gtk::Button("解锁 Bootloader"));
    button_box->pack_start(*m_btn_unlock, false, false, 5);

    m_btn_confirm_unlock = Gtk::manage(new Gtk::Button("确认解锁"));
    m_btn_confirm_unlock->set_sensitive(false);
    button_box->pack_start(*m_btn_confirm_unlock, false, false, 5);

    m_btn_reboot_after_unlock = Gtk::manage(new Gtk::Button("重启设备"));
    m_btn_reboot_after_unlock->set_sensitive(false);
    button_box->pack_start(*m_btn_reboot_after_unlock, false, false, 5);

    m_btn_reboot_device = Gtk::manage(new Gtk::Button("重启"));
    button_box->pack_start(*m_btn_reboot_device, false, false, 5);

    m_btn_flash = Gtk::manage(new Gtk::Button("开始烧录"));
    m_btn_flash->set_sensitive(false);
    button_box->pack_start(*m_btn_flash, false, false, 5);

    center_grid->attach(*button_box, 0, 1, 4, 1);
    m_main_vbox->pack_start(*center_grid, false, false, 5);

    m_main_vbox->pack_start(*Gtk::manage(new Gtk::HSeparator()), false, false, 5);

    auto log_header_box = Gtk::manage(new Gtk::HBox(false, 5));
    log_header_box->pack_start(*Gtk::manage(new Gtk::Label("操作日志:")), false, false, 5);

    m_btn_toggle_log = Gtk::manage(new Gtk::Button("收起"));
    m_btn_toggle_log->set_hexpand(false);
    log_header_box->pack_end(*m_btn_toggle_log, false, false, 5);

    m_main_vbox->pack_start(*log_header_box, false, false, 5);

    m_log_frame = Gtk::manage(new Gtk::Frame());
    m_log_frame->set_shadow_type(Gtk::SHADOW_ETCHED_IN);
    m_log_frame->set_hexpand(true);
    m_log_frame->set_vexpand(true);

    auto scrolled_window = Gtk::manage(new Gtk::ScrolledWindow());
    scrolled_window->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    scrolled_window->set_size_request(-1, 180);

    m_log_view = Gtk::manage(new Gtk::TextView());
    m_log_view->set_editable(false);
    m_log_view->set_cursor_visible(false);
    m_log_buffer = m_log_view->get_buffer();
    scrolled_window->add(*m_log_view);
    m_log_frame->add(*scrolled_window);
    m_main_vbox->pack_start(*m_log_frame, true, true, 0);

    // 添加操作说明标签
    m_instructions_label = Gtk::manage(new Gtk::Label());
    m_instructions_label->set_markup(
        "<b>使用步骤:</b>\n" 
        "1. 点击\"选择镜像\"按钮选择 System.img 文件\n" 
        "2. 点击\"检测设备\"确认设备已连接\n" 
        "3. 如需解锁，点击\"解锁 Bootloader\"并按设备提示操作\n" 
        "4. 点击\"开始烧录\"将镜像写入设备\n\n" 
        "<b>注意事项:</b>\n" 
        "  - 确保设备已开启 USB 调试\n" 
        "  - 解锁和烧录会清除设备数据\n" 
        "  - 请保持设备连接直到操作完成"
    );
    m_instructions_label->set_justify(Gtk::JUSTIFY_LEFT);
    m_instructions_label->set_line_wrap(true);
    m_instructions_label->set_visible(false);
    m_main_vbox->pack_start(*m_instructions_label, true, true, 0);

    m_status_bar = Gtk::manage(new StatusBar());
    m_main_vbox->pack_end(*m_status_bar, false, false, 0);

    m_main_vbox->show_all();
    m_instructions_label->hide();
}

void MainWindow::connect_signals() {
    m_file_chooser->signal_file_set().connect(sigc::mem_fun(*this, &MainWindow::on_file_choose));
    m_btn_detect->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_detect_clicked));
    m_btn_unlock->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_unlock_clicked));
    m_btn_confirm_unlock->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_confirm_unlock_clicked));
    m_btn_reboot_after_unlock->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_reboot_after_unlock_clicked));
    m_btn_reboot_device->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_reboot_device_clicked));
    m_btn_flash->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_flash_clicked));
    m_btn_toggle_log->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_toggle_log_clicked));

    m_flasher->signal_status_update().connect(sigc::mem_fun(*this, &MainWindow::on_status_update));
    m_flasher->signal_log_message().connect(sigc::mem_fun(*this, &MainWindow::on_log_message));
    m_flasher->signal_user_action_required().connect(sigc::mem_fun(*this, &MainWindow::on_user_action_required));
    m_flasher->signal_finished().connect(sigc::mem_fun(*this, &MainWindow::on_finished));
    m_flasher->signal_unlock_complete_waiting_reboot().connect(sigc::mem_fun(*this, &MainWindow::on_unlock_complete_waiting_reboot));
    m_flasher->signal_progress().connect(sigc::mem_fun(*this, &MainWindow::on_progress));
}

void MainWindow::on_file_choose() {
    std::string filename = m_file_chooser->get_filename();
    if (!filename.empty()) {
        m_flasher->select_image(filename);
        m_btn_flash->set_sensitive(true);
    }
}

void MainWindow::on_detect_clicked() {
    append_log("正在检测设备...", "blue");
    m_btn_detect->set_sensitive(false);

    auto worker = std::make_unique<ThreadWorker>();
    auto worker_ptr = worker.get();

    worker_ptr->run_async([this]() {
        auto adb = std::make_unique<AdbHelper>();
        auto fastboot = std::make_unique<FastbootHelper>();

        adb->set_log_callback([this](const std::string& msg) {
            Glib::signal_idle().connect_once([this, msg]() {
                on_log_message(msg);
            });
        });
        fastboot->set_log_callback([this](const std::string& msg) {
            Glib::signal_idle().connect_once([this, msg]() {
                on_log_message(msg);
            });
        });

        auto adb_state = adb->get_device_state();
        auto fastboot_devices = fastboot->list_devices();

        Glib::signal_idle().connect_once([this, adb_state, fastboot_devices]() {
            if (!adb_state.empty()) {
                on_status_update("检测到 ADB 设备: " + adb_state, FlasherStatus::SUCCESS);
                append_log("ADB 设备状态: " + adb_state, "green");
            } else if (!fastboot_devices.empty()) {
                on_status_update("检测到 Fastboot 设备", FlasherStatus::SUCCESS);
                append_log("Fastboot 设备:", "green");
                for (const auto& dev : fastboot_devices) {
                    append_log("  " + dev, "green");
                }
            } else {
                on_status_update("未检测到设备", FlasherStatus::ERROR);
                append_log("未检测到设备，请检查 USB 连接和调试设置", "red");
            }
            m_btn_detect->set_sensitive(true);
        });
    });

    worker.release();
}

void MainWindow::on_unlock_clicked() {
    Gtk::MessageDialog dialog(*this, "确认解锁 Bootloader", false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK_CANCEL, true);
    dialog.set_secondary_text(
        "警告：解锁 Bootloader 将清除设备上的所有数据！\n"
        "此操作可能会使设备保修失效。\n\n"
        "确定要继续吗？"
    );

    if (dialog.run() == Gtk::RESPONSE_OK) {
        m_btn_unlock->set_sensitive(false);
        m_btn_flash->set_sensitive(false);
        m_flasher->unlock_device();
    }
}

void MainWindow::on_confirm_unlock_clicked() {
    m_btn_confirm_unlock->set_sensitive(false);
    m_flasher->confirm_unlock();
}

void MainWindow::on_reboot_after_unlock_clicked() {
    m_btn_reboot_after_unlock->set_sensitive(false);
    m_btn_unlock->set_sensitive(false);
    m_btn_flash->set_sensitive(false);
    m_flasher->reboot_after_unlock();
}

void MainWindow::on_reboot_device_clicked() {
    append_log("正在检测设备状态...", "blue");
    m_btn_reboot_device->set_sensitive(false);

    auto worker = std::make_unique<ThreadWorker>();
    auto worker_ptr = worker.get();

    worker_ptr->run_async([this]() {
        auto adb = std::make_unique<AdbHelper>();
        auto fastboot = std::make_unique<FastbootHelper>();

        bool adb_ok = adb->is_device_connected();
        bool fastboot_ok = fastboot->is_device_connected();

        Glib::signal_idle().connect_once([this, adb_ok, fastboot_ok]() {
            if (adb_ok) {
                append_log("使用 ADB 重启设备...", "blue");
                m_adb->reboot();
                on_status_update("正在重启设备...", FlasherStatus::WORKING);
            } else if (fastboot_ok) {
                append_log("使用 Fastboot 重启设备...", "blue");
                m_fastboot->reboot();
                on_status_update("正在重启设备...", FlasherStatus::WORKING);
            } else {
                append_log("未检测到设备，无法重启", "red");
                on_status_update("未检测到设备", FlasherStatus::ERROR);
            }
            m_btn_reboot_device->set_sensitive(true);
        });
    });

    worker.release();
}

void MainWindow::on_flash_clicked() {
    Gtk::MessageDialog dialog(*this, "确认烧录", false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK_CANCEL, true);
    dialog.set_secondary_text(
        "警告：烧录将清除设备上的所有数据！\n\n"
        "请确保：\n"
        "  - 已选择正确的 System.img 文件\n"
        "  - 设备已连接\n"
        "  - 设备 Bootloader 已解锁（如需要）\n\n"
        "确定要开始烧录吗？"
    );

    if (dialog.run() == Gtk::RESPONSE_OK) {
        m_btn_unlock->set_sensitive(false);
        m_btn_flash->set_sensitive(false);
        m_flasher->flash_system();
    }
}

void MainWindow::on_toggle_log_clicked() {
    m_log_visible = !m_log_visible;
    if (m_log_visible) {
        m_log_frame->show();
        m_instructions_label->hide();
        m_btn_toggle_log->set_label("收起");
    } else {
        m_log_frame->hide();
        m_instructions_label->show();
        m_btn_toggle_log->set_label("展开");
    }
}

void MainWindow::on_show_instructions_clicked() {
    Gtk::MessageDialog dialog(*this, "操作说明", false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, true);
    dialog.set_secondary_text(
        "<b>使用步骤:</b>\n\n"
        "1. 点击\"选择镜像\"按钮选择 System.img 文件\n"
        "2. 点击\"检测设备\"确认设备已连接\n"
        "3. 如需解锁，点击\"解锁 Bootloader\"并按设备提示操作\n"
        "4. 点击\"开始烧录\"将镜像写入设备\n\n"
        "<b>注意事项:</b>\n\n"
        "  - 确保设备已开启 USB 调试\n"
        "  - 解锁和烧录会清除设备数据\n"
        "  - 请保持设备连接直到操作完成"
    );
    dialog.run();
}

void MainWindow::on_unlock_complete_waiting_reboot() {
    m_btn_reboot_after_unlock->set_sensitive(true);
    m_btn_unlock->set_sensitive(false);
    m_btn_confirm_unlock->set_sensitive(false);
}

void MainWindow::on_status_update(const std::string& message, FlasherStatus status) {
    m_status_bar->set_status(message, status);
}

void MainWindow::on_log_message(const std::string& message) {
    append_log(message, "");
}

void MainWindow::on_user_action_required(const std::string& message) {
    append_log(message, "orange");
    m_btn_confirm_unlock->set_sensitive(true);

    Gtk::MessageDialog dialog(*this, "需要用户操作", false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, true);
    dialog.set_secondary_text(message);
    dialog.run();
}

void MainWindow::on_finished(bool success) {
    m_btn_unlock->set_sensitive(true);
    m_btn_confirm_unlock->set_sensitive(false);
    m_btn_reboot_after_unlock->set_sensitive(false);
    if (!m_flasher->get_image_path().empty()) {
        m_btn_flash->set_sensitive(true);
    }

    if (success) {
        Gtk::MessageDialog dialog(*this, "操作完成", false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, true);
        dialog.set_secondary_text("操作已成功完成！");
        dialog.run();
    }
}

void MainWindow::on_logger_message(LogLevel level, const std::string& message) {
    std::string color;
    switch (level) {
        case LogLevel::INFO: color = ""; break;
        case LogLevel::WARNING: color = "orange"; break;
        case LogLevel::ERROR: color = "red"; break;
        case LogLevel::SUCCESS: color = "green"; break;
    }
    append_log(message, color);
}

void MainWindow::on_progress(const std::string& message) {
    append_log(message, "blue");
}

void MainWindow::append_log(const std::string& text, const std::string& color) {
    auto iter = m_log_buffer->end();
    std::string mark = "[" + Glib::DateTime::create_now_local().format("%H:%M:%S") + "] ";

    m_log_buffer->insert(iter, mark);
    iter = m_log_buffer->end();

    if (color.empty()) {
        m_log_buffer->insert(iter, text + "\n");
    } else {
        auto tag = m_log_buffer->create_tag();
        tag->property_foreground() = color;
        m_log_buffer->insert_with_tag(iter, text + "\n", tag);
    }

    auto mark_iter = m_log_buffer->end();
    m_log_view->scroll_to(mark_iter);
}

void MainWindow::show_about_dialog() {
    Gtk::AboutDialog dialog;
    dialog.set_transient_for(*this);
    dialog.set_program_name("System Img Flasher");
    dialog.set_version("1.0");
    dialog.set_comments("用于将 System.img 烧录到 Android 设备的 GUI 工具");
    dialog.set_copyright("Copyright © 2026");
    dialog.set_license_type(Gtk::LICENSE_GPL_3_0);
    dialog.run();
}
