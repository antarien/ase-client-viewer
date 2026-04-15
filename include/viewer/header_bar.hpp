#pragma once

/**
 * @file        header_bar.hpp
 * @brief       Titlebar with title label, search entry, glyph buttons
 * @description Mirrors clients/ase-client-explorer/src/window.cpp header
 *              mechanics 1:1: title label + centred search entry (swapped
 *              in when search toggles open), right-aligned cluster of
 *              flat glyph buttons sourced from the generated ui_icons.hpp
 *              SSOT (icon-definitions.ts). No title buttons (minimise /
 *              maximise / close) — the app closes via the window-level
 *              Escape handler installed in ViewerWindow::build_ui.
 *
 *              TECH / DSGN mode toggles live at pack_start and keep the
 *              sidebar's subfolder swap working. Everything else (search,
 *              mark-all-read, refresh, settings) is a glyph button.
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

    /**
     * Toggle the search entry on/off. When visible, the title label is
     * hidden and the entry fills the header's centre title slot; when
     * hidden, the title label is restored.
     */
    void toggle_search(bool visible);

    /** True while the search entry is currently visible. */
    bool search_visible() const noexcept { return m_search_visible; }

    /**
     * Seed the entry with a single UTF-8 character and move the cursor
     * past it — used by the type-ahead handler so printable keystrokes
     * produce the expected text order instead of inserting before the
     * seed character.
     */
    void seed_search_with(const std::string& utf8_char);

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

    /** Fired when the search magnifier button is clicked. */
    template <typename Callback>
    void on_search_toggle(Callback&& cb) {
        m_on_search_toggle = sigc::slot<void()>(
            [fn = std::forward<Callback>(cb)]() { fn(); });
    }

    /** Fired when the "mark all read" button is clicked. */
    template <typename Callback>
    void on_mark_all_read(Callback&& cb) {
        m_on_mark = sigc::slot<void()>(
            [fn = std::forward<Callback>(cb)]() { fn(); });
    }

    /** Fired when the refresh button is clicked. */
    template <typename Callback>
    void on_refresh(Callback&& cb) {
        m_on_refresh = sigc::slot<void()>(
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
    Gtk::HeaderBar    m_header;
    Gtk::Label        m_title_label;
    Gtk::Box          m_mode_box{Gtk::Orientation::HORIZONTAL, 0};
    Gtk::ToggleButton m_btn_tech;
    Gtk::ToggleButton m_btn_dsgn;
    Gtk::SearchEntry  m_search;
    Gtk::Button       m_btn_search;
    Gtk::Button       m_btn_mark_read;
    Gtk::Button       m_btn_refresh;
    Gtk::Button       m_btn_settings;

    sigc::slot<void(uint8_t)>            m_on_mode;
    sigc::slot<void(const std::string&)> m_on_search;
    sigc::slot<void()>                   m_on_search_toggle;
    sigc::slot<void()>                   m_on_mark;
    sigc::slot<void()>                   m_on_refresh;
    sigc::slot<void()>                   m_on_settings;

    bool m_suppress_mode_signal = false;
    bool m_search_visible       = false;
};

}  // namespace ase::viewer
