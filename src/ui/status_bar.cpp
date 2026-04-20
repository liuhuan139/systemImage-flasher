#include "status_bar.hpp"
#include <cmath>

StatusBar::StatusBar()
    : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 5)
    , m_current_status(FlasherStatus::IDLE)
{
    set_border_width(5);

    m_status_label = Gtk::manage(new Gtk::Label("就绪"));
    m_status_label->set_halign(Gtk::ALIGN_START);
    pack_start(*m_status_label, true, true, 5);

    m_indicator = Gtk::manage(new Gtk::DrawingArea());
    m_indicator->set_size_request(24, 24);
    m_indicator->signal_draw().connect(sigc::mem_fun(*this, &StatusBar::on_indicator_draw));
    pack_start(*m_indicator, false, false, 5);
}

void StatusBar::set_status(const std::string& text, FlasherStatus status) {
    m_status_label->set_text(text);
    m_current_status = status;
    m_indicator->queue_draw();
}

bool StatusBar::on_indicator_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
    Gtk::Allocation allocation = m_indicator->get_allocation();
    const int width = allocation.get_width();
    const int height = allocation.get_height();
    const int radius = std::min(width, height) / 2 - 2;
    const int center_x = width / 2;
    const int center_y = height / 2;

    cr->arc(center_x, center_y, radius, 0.0, 2.0 * M_PI);

    switch (m_current_status) {
        case FlasherStatus::IDLE:
            cr->set_source_rgba(0.5, 0.5, 0.5, 1.0);
            break;
        case FlasherStatus::WORKING:
            cr->set_source_rgba(0.2, 0.4, 0.8, 1.0);
            break;
        case FlasherStatus::SUCCESS:
            cr->set_source_rgba(0.2, 0.8, 0.2, 1.0);
            break;
        case FlasherStatus::ERROR:
            cr->set_source_rgba(0.8, 0.2, 0.2, 1.0);
            break;
        case FlasherStatus::WAITING_USER:
            cr->set_source_rgba(0.9, 0.7, 0.1, 1.0);
            break;
        case FlasherStatus::WARNING:
            cr->set_source_rgba(1.0, 0.5, 0.0, 1.0);
            break;
    }

    cr->fill_preserve();

    cr->set_line_width(1.0);
    cr->set_source_rgba(0.0, 0.0, 0.0, 0.3);
    cr->stroke();

    return true;
}
