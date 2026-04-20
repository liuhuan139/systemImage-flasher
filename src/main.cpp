#include <gtkmm.h>
#include "ui/main_window.hpp"

int main(int argc, char* argv[]) {
    auto app = Gtk::Application::create(argc, argv, "com.example.flasher");

    MainWindow window;
    window.set_position(Gtk::WIN_POS_CENTER);

    return app->run(window);
}
