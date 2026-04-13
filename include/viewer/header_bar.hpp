#pragma once

/**
 * @file        header_bar.hpp
 * @brief       Titlebar with mode toggles, search entry, action buttons
 * @description Encapsulates the viewer's top header: ASE title label,
 *              TECH/DSGN mode toggle pair, search entry, mark-all-read
 *              and settings buttons. Every interaction is exposed to the
 *              orchestrator through on_*() callback slots so this file
 *              owns the widgets but zero of the behaviour.
 *
 *              Kept as raw gtkmm because ase::gtk::HeaderBar wraps only
 *              Gtk::HeaderBar itself, while ToggleButton is not yet in
 *              the adapter's surface.
 *
 * @module      ase-client-viewer
 * @layer       5
 */

#include <gtkmm/headerbar.h>
#include <gtkmm/label.h>
#include <gtkmm/togglebutton.h>
#include <gtkmm/button.h>
#include <gtkmm/searchentry.h>
#include <gtkmm/box.h>
#include <sigc++/slot.h>

#include <cstdint>
#include <string>
#include <utility>

namespace ase::viewer {

class HeaderBar {
public:
    HeaderBar();

    /** Return the gtkmm widget for installation as the window titlebar. */
    Gtk::HeaderBar& widget() noexcept { return m_header; }
    Gtk::Widget* native_widget() noexcept { return &m_header; }

    /** Update the displayed title label. */
    void set_title(const std::string& title);

    /** Force the active mode button without firing on_mode_changed. */
    void set_mode(uint8_t mode);

    /** Fired when the user toggles TECH (0) / DSGN (1). */
    template <typename Callback>
    void on_mode_changed(Callback&& cb) {
        m_on_mode = sigc::slot<void(uint8_t)>(
            [fn = std::forward<Callback>(cb)](uint8_t mode) { fn(mode); });
    }

    /** Fired for every keystroke in the search entry, lowercased. */
    template <typename Callback>
    void on_search_text(Callback&& cb) {
        m_on_search = sigc::slot<void(const std::string&)>(
            [fn = std::forward<Callback>(cb)](const std::string& s) { fn(s); });
    }

    /** Fired when the "mark all read" button is clicked. */
    template <typename Callback>
    void on_mark_all_read(Callback&& cb) {
        m_on_mark = sigc::slot<void()>(
            [fn = std::forward<Callback>(cb)]() { fn(); });
    }

    /** Fired when the settings gear is clicked. */
    template <typename Callback>
    void on_settings(Callback&& cb) {
        m_on_settings = sigc::slot<void()>(
            [fn = std::forward<Callback>(cb)]() { fn(); });
    }

    /** Move keyboard focus to the search entry (Ctrl+F handler). */
    void focus_search();

    /** Clear the search entry text (Escape handler). */
    void clear_search();

private:
    Gtk::HeaderBar   m_header;
    Gtk::Label       m_title_label;
    Gtk::Box         m_mode_box{Gtk::Orientation::HORIZONTAL, 0};
    Gtk::ToggleButton m_btn_tech;
    Gtk::ToggleButton m_btn_dsgn;
    Gtk::SearchEntry m_search;
    Gtk::Button      m_btn_mark_read;
    Gtk::Button      m_btn_settings;

    sigc::slot<void(uint8_t)>            m_on_mode;
    sigc::slot<void(const std::string&)> m_on_search;
    sigc::slot<void()>                   m_on_mark;
    sigc::slot<void()>                   m_on_settings;

    // True while set_mode() is programmatically flipping the toggle
    // buttons. The toggled-signal handler checks this flag and skips
    // firing on_mode_changed to break the recursion when the window
    // orchestrator calls set_mode from inside open_file or similar.
    bool m_suppress_mode_signal = false;
};

}  // namespace ase::viewer
