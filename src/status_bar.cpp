/**
 * @file        status_bar.cpp
 * @brief       StatusBar implementation - label left, progress bar right
 * @description Builds the one horizontal Box the status bar owns: a
 *              left-aligned, expanding Label and a right-aligned ProgressBar.
 *              Until a document is loaded the label reads "No document", so the
 *              bar never shows an empty field whose meaning nobody can tell.
 *
 * @module      ase-client-viewer
 * @layer       5 (Servers & Clients)
 * @category    render/ui/widget
 * @usecase     usecase:100008 - render the knowledge stock
 * @created     2026-04-13
 * @modified    2026-10-13
 * @version     00.00.35.00035 [core]
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
