#pragma once

/**
 * @file        keyboard.hpp
 * @brief       Window-level keyboard shortcut controller
 * @description Wraps a Gtk::EventControllerKey so the orchestrator can
 *              register slots for the shortcuts the viewer cares about:
 *              Ctrl+F (focus search), Escape (clear search), F5 (refresh),
 *              Ctrl+, (settings), Ctrl+1/2 (mode toggles). The window
 *              installs the controller once via install_on().
 *
 * @module      ase-client-viewer
 * @layer       5
 */

#include <gtkmm/eventcontrollerkey.h>
#include <gtkmm/widget.h>
#include <glibmm/refptr.h>
#include <sigc++/slot.h>

#include <cstdint>
#include <utility>

namespace ase::viewer {

class KeyboardShortcuts {
public:
    KeyboardShortcuts();

    /** Attach the controller to the given widget (typically the window). */
    void install_on(Gtk::Widget& widget);

    /** Ctrl+F - focus the search entry. */
    template <typename Callback>
    void on_focus_search(Callback&& cb) {
        m_on_focus_search = sigc::slot<void()>(
            [fn = std::forward<Callback>(cb)]() { fn(); });
    }

    /** Escape - abort search and return focus to the tree. */
    template <typename Callback>
    void on_escape(Callback&& cb) {
        m_on_escape = sigc::slot<void()>(
            [fn = std::forward<Callback>(cb)]() { fn(); });
    }

    /** F5 - refresh the tree. */
    template <typename Callback>
    void on_refresh(Callback&& cb) {
        m_on_refresh = sigc::slot<void()>(
            [fn = std::forward<Callback>(cb)]() { fn(); });
    }

    /** Ctrl+, - open the settings dialog. */
    template <typename Callback>
    void on_settings(Callback&& cb) {
        m_on_settings = sigc::slot<void()>(
            [fn = std::forward<Callback>(cb)]() { fn(); });
    }

    /** Ctrl+1 / Ctrl+2 - switch to TECH (0) / DSGN (1). */
    template <typename Callback>
    void on_mode(Callback&& cb) {
        m_on_mode = sigc::slot<void(uint8_t)>(
            [fn = std::forward<Callback>(cb)](uint8_t mode) { fn(mode); });
    }

    /** Ctrl+Shift+C - copy the current document's text dump to clipboard. */
    template <typename Callback>
    void on_copy_dump(Callback&& cb) {
        m_on_copy_dump = sigc::slot<void()>(
            [fn = std::forward<Callback>(cb)]() { fn(); });
    }

private:
    Glib::RefPtr<Gtk::EventControllerKey> m_controller;

    sigc::slot<void()>        m_on_focus_search;
    sigc::slot<void()>        m_on_escape;
    sigc::slot<void()>        m_on_refresh;
    sigc::slot<void()>        m_on_settings;
    sigc::slot<void(uint8_t)> m_on_mode;
    sigc::slot<void()>        m_on_copy_dump;
};

}  // namespace ase::viewer
