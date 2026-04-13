/**
 * @file        status_bar.cpp
 * @brief       StatusBar implementation
 *
 * @module      ase-client-viewer
 * @layer       5
 */

#include <viewer/status_bar.hpp>

namespace ase::viewer {

StatusBar::StatusBar() {
    m_box.add_css_class("status-bar");

    m_label.set_text("No document");
    m_label.set_hexpand(true);
    m_label.set_xalign(0);
    m_box.append(m_label);

    m_progress.set_fraction(0);
    m_progress.set_valign(Gtk::Align::CENTER);
    m_progress.set_size_request(120, 4);
    m_box.append(m_progress);
}

void StatusBar::set_text(const std::string& text) {
    m_label.set_text(text);
}

void StatusBar::set_progress(double fraction) {
    m_progress.set_fraction(fraction);
}

}  // namespace ase::viewer
