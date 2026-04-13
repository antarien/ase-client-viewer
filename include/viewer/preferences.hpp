#pragma once

/**
 * @file        preferences.hpp
 * @brief       Modal Adwaita preferences window with three pages
 * @description Renders an AdwPreferencesWindow with Appearance, Documents
 *              and Viewer pages. Editing any row persists through
 *              save_settings() and notifies the caller so the main canvas
 *              can redraw with the new values.
 *
 * @module      ase-client-viewer
 * @layer       5
 */

#include <viewer/settings.hpp>

#include <adwaita.h>
#include <gtkmm/window.h>
#include <sigc++/slot.h>

#include <string>
#include <utility>

namespace ase::viewer {

/**
 * Build and show the preferences window as transient for parent. The
 * on_changed slot is invoked after every persisted change so the caller
 * can queue a redraw of anything affected by the setting.
 */
void show_preferences_window(
    Gtk::Window& parent,
    ViewerSettings& settings,
    const std::string& config_dir,
    sigc::slot<void()> on_changed);

}  // namespace ase::viewer
