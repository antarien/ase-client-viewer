/**
 * @file        keyboard.cpp
 * @brief       KeyboardShortcuts implementation
 * @description Installs a single EventControllerKey that routes the window's
 *              shortcuts to the registered slots. Returns true from the
 *              key_pressed handler when the event is consumed so gtkmm
 *              stops propagating it.
 *
 * @module      ase-client-viewer
 * @layer       5
 */

#include <viewer/keyboard.hpp>

#include <gdk/gdkkeysyms.h>

namespace ase::viewer {

KeyboardShortcuts::KeyboardShortcuts()
    : m_controller(Gtk::EventControllerKey::create())
{
    m_controller->signal_key_pressed().connect(
        [this](guint keyval, guint, Gdk::ModifierType state) -> bool {
            const bool ctrl  = (state & Gdk::ModifierType::CONTROL_MASK) != Gdk::ModifierType{};
            const bool shift = (state & Gdk::ModifierType::SHIFT_MASK)   != Gdk::ModifierType{};

            // Ctrl+Shift+C needs to run BEFORE the plain Ctrl+C/Ctrl+F
            // branches because the bare keyval for Shift+C is GDK_KEY_C
            // (uppercase) while without shift it is GDK_KEY_c.
            if (ctrl && shift && (keyval == GDK_KEY_C || keyval == GDK_KEY_c)) {
                if (m_on_copy_dump) m_on_copy_dump();
                return true;
            }

            if (ctrl && keyval == GDK_KEY_f) {
                if (m_on_focus_search) m_on_focus_search();
                return true;
            }
            if (keyval == GDK_KEY_Escape) {
                if (m_on_escape) m_on_escape();
                return true;
            }
            if (keyval == GDK_KEY_F5) {
                if (m_on_refresh) m_on_refresh();
                return true;
            }
            if (ctrl && keyval == GDK_KEY_comma) {
                if (m_on_settings) m_on_settings();
                return true;
            }
            if (ctrl && keyval == GDK_KEY_1) {
                if (m_on_mode) m_on_mode(0u);
                return true;
            }
            if (ctrl && keyval == GDK_KEY_2) {
                if (m_on_mode) m_on_mode(1u);
                return true;
            }
            return false;
        }, false);
}

void KeyboardShortcuts::install_on(Gtk::Widget& widget) {
    widget.add_controller(m_controller);
}

}  // namespace ase::viewer
