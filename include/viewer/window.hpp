#pragma once

/**
 * @file        window.hpp
 * @brief       ViewerWindow - orchestrator that composes every feature
 * @description Owns one instance of each UI feature (HeaderBar, TreeView,
 *              Canvas, StatusBar, KeyboardShortcuts, FileMonitor,
 *              ReadTracker, ViewerSettings) plus the top-level
 *              ase::adp::gtk::ApplicationWindow. build_ui() wires everything
 *              together; load_directory() / load_file() are the two
 *              state-mutating entry points everyone else routes through.
 *
 *              This class is the ONLY place that knows about all the
 *              feature slices - each feature file is independent and
 *              delegates back to the window via sigc::slot callbacks
 *              installed during build.
 *
 * @module      ase-client-viewer
 * @layer       5
 */

#include <viewer/canvas.hpp>
#include <viewer/file_monitor.hpp>
#include <viewer/header_bar.hpp>
#include <viewer/keyboard.hpp>
#include <viewer/read_tracker.hpp>
#include <viewer/settings.hpp>
#include <viewer/status_bar.hpp>
#include <viewer/tree_view.hpp>

#include <ase/adp/gtk/application.hpp>

#include <gtkmm/box.h>
#include <gtkmm/paned.h>

#include <cstdint>
#include <string>

namespace ase::viewer {

class ViewerWindow {
public:
    explicit ViewerWindow(ase::adp::gtk::ApplicationWindow window);

    /** Assemble the full UI: header, tree, canvas, status bar, shortcuts. */
    void build_ui();

    /** Load a directory as the new tree root. */
    void load_directory(const std::string& path);

    /** Load a single markdown file. Uses its parent directory as the tree root. */
    void load_file(const std::string& path);

    /** Forward to the underlying ApplicationWindow. */
    void present();

private:
    // Handlers invoked by the feature slices via their stored slots.
    void handle_file_activated(const std::string& path);
    void handle_mode_changed(uint8_t mode);
    void handle_search_changed(const std::string& lowered);
    void handle_mark_all_read();
    void handle_open_settings();
    void handle_refresh();
    void handle_escape();
    void handle_focus_search();
    void handle_copy_dump();

    // Internal helpers.
    void open_file(const std::string& path);
    void switch_mode_directory(uint8_t target_mode);
    void resolve_config_dir();
    void setup_file_watch(const std::string& root);
    void update_window_title(const std::string& subtitle);
    void update_status_bar_for_current();

    // Declaration order matters: m_read_tracker is passed by reference
    // into m_tree_view's constructor, so it MUST be declared first.
    ReadTracker    m_read_tracker;
    ViewerSettings m_settings;
    std::string    m_config_dir;
    std::string    m_current_file;

    ase::adp::gtk::ApplicationWindow m_window;

    HeaderBar         m_header;
    TreeView          m_tree_view;
    Canvas            m_canvas;
    StatusBar         m_status_bar;
    KeyboardShortcuts m_shortcuts;
    FileMonitor       m_file_monitor;

    Gtk::Paned m_paned{Gtk::Orientation::HORIZONTAL};
    Gtk::Box   m_main_box{Gtk::Orientation::VERTICAL, 0};
};

}  // namespace ase::viewer
