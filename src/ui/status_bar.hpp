#ifndef STATUS_BAR_HPP
#define STATUS_BAR_HPP

#include <gtkmm.h>
#include "../core/flasher.hpp"

class StatusBar : public Gtk::Box {
public:
    StatusBar();
    ~StatusBar() = default;

    void set_status(const std::string& text, FlasherStatus status);

private:
    bool on_indicator_draw(const Cairo::RefPtr<Cairo::Context>& cr);

    Gtk::Label* m_status_label;
    Gtk::DrawingArea* m_indicator;
    FlasherStatus m_current_status;
};

#endif // STATUS_BAR_HPP
