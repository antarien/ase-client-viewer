#pragma once

/**
 * @file        status_bar.hpp
 * @brief       Bottom status bar: filename / word count / progress
 * @description Single responsibility: own one horizontal Box containing a
 *              left-aligned Label and a right-aligned ProgressBar. The
 *              window orchestrator calls set_text() after a document is
 *              loaded and set_progress() during long operations.
 *
 * @module      ase-client-viewer
 * @layer       5
 */

#include <gtkmm/box.h>
#include <gtkmm/label.h>
#include <gtkmm/progressbar.h>

#include <string>

namespace ase::viewer {

class StatusBar {
public:
    StatusBar();

    /** Return the top-level widget for insertion into a parent container. */
    Gtk::Box& widget() noexcept { return m_box; }
    Gtk::Widget* native_widget() noexcept { return &m_box; }

    /** Replace the status line text. */
    void set_text(const std::string& text);

    /** Set progress bar fraction in [0, 1]. */
    void set_progress(double fraction);

private:
    Gtk::Box        m_box{Gtk::Orientation::HORIZONTAL, 8};
    Gtk::Label      m_label;
    Gtk::ProgressBar m_progress;
};

}  // namespace ase::viewer
