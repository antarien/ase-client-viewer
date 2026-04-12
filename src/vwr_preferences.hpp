#pragma once

/**
 * Preferences window — Adw::PreferencesWindow with three pages.
 *
 * Pages: Appearance, Documents, Viewer
 * Reads/writes ViewerSettings, persists on close.
 */

#include "vwr_settings.hpp"
#include <adwaita.h>
#include <gtkmm.h>
#include <functional>

namespace ase::viewer {

// Build and show preferences window (libadwaita native C API).
// on_changed is called whenever a setting changes.
void show_preferences_window(
    Gtk::Window& parent,
    ViewerSettings& settings,
    const std::string& config_dir,
    const std::function<void()>& on_changed);

}  // namespace ase::viewer
