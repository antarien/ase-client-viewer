/**
 * @file        window.cpp
 * @brief       ViewerWindow implementation - build, wire, route
 * @description build_ui() composes the header + Paned(tree | canvas) +
 *              status bar and hooks every feature slot to one of the
 *              handle_* methods below. The handlers are the ONLY place
 *              that mutates multiple features at once - individual
 *              feature files never reach across to each other.
 *
 * @module      ase-client-viewer
 * @layer       5
 */

#include <viewer/window.hpp>

#include <viewer/fs_utils.hpp>
#include <viewer/preferences.hpp>

#include <ase/markdown/markdown.hpp>

#include <gdkmm/clipboard.h>
#include <glibmm/refptr.h>

#include <cctype>
#include <cstdlib>
#include <string>
#include <utility>

namespace ase::viewer {

namespace {

std::string compute_status_text(const std::string& path,
                                const std::string& content,
                                uint8_t mode) {
    int word_count = 0;
    bool in_word = false;
    for (char c : content) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            if (!in_word) { ++word_count; in_word = true; }
        } else {
            in_word = false;
        }
    }
    const int read_min = (word_count + 199) / 200;
    const std::string mode_str = mode == 0 ? "TECH" : "DSGN";
    return fs::base_name(path) + "  |  " +
           std::to_string(word_count) + " words  |  ~" +
           std::to_string(read_min) + " min read  |  " + mode_str;
}

}  // namespace

ViewerWindow::ViewerWindow(ase::gtk::ApplicationWindow window)
    : m_window(std::move(window))
    , m_tree_view(m_read_tracker)
{}

void ViewerWindow::build_ui() {
    m_window.set_title("ASE TECH & DESIGN Viewer");
    m_window.set_default_size(1400, 900);

    resolve_config_dir();
    m_read_tracker.load(m_config_dir);
    m_settings = load_settings(m_config_dir);

    // Header and canvas BOTH track the same mode value. Any drift
    // between them means files open under the wrong directive parser
    // and raw :::name{attrs} leaks into the rendered output.
    m_header.set_mode(m_settings.default_mode);
    m_canvas.set_mode(m_settings.default_mode);

    // ── Header bar ──
    m_window.set_titlebar(m_header);

    m_header.on_mode_changed([this](uint8_t mode) {
        handle_mode_changed(mode);
    });
    m_header.on_search_text([this](const std::string& lowered) {
        handle_search_changed(lowered);
    });
    m_header.on_mark_all_read([this]() {
        handle_mark_all_read();
    });
    m_header.on_settings([this]() {
        handle_open_settings();
    });

    // ── Tree view (left) ──
    m_paned.set_position(280);
    m_paned.set_shrink_start_child(false);
    m_paned.set_vexpand(true);
    m_paned.set_start_child(m_tree_view.widget());

    m_tree_view.on_file_activated([this](const std::string& path) {
        handle_file_activated(path);
    });

    // ── Canvas (right) ──
    m_paned.set_end_child(m_canvas.widget());

    // ── Main vertical layout: paned + status bar ──
    // Layout widgets (Gtk::Box, Gtk::Paned) are NOT in the ase::gtk
    // adapter, so we drive them through raw gtkmm calls on the underlying
    // Gtk::ApplicationWindow pointer.
    m_main_box.append(m_paned);
    m_main_box.append(m_status_bar.widget());
    m_window.native()->set_child(m_main_box);

    // ── Keyboard shortcuts ──
    m_shortcuts.on_focus_search([this]() { handle_focus_search(); });
    m_shortcuts.on_escape      ([this]() { handle_escape(); });
    m_shortcuts.on_refresh     ([this]() { handle_refresh(); });
    m_shortcuts.on_settings    ([this]() { handle_open_settings(); });
    m_shortcuts.on_mode([this](uint8_t mode) { handle_mode_changed(mode); });
    m_shortcuts.on_copy_dump   ([this]() { handle_copy_dump(); });
    m_shortcuts.install_on(*m_window.native_widget());
}

void ViewerWindow::load_directory(const std::string& path) {
    m_tree_view.load(path);
    setup_file_watch(path);
    update_window_title(path);
}

void ViewerWindow::load_file(const std::string& path) {
    const std::string parent = fs::parent_dir(path);
    m_tree_view.load(parent);
    setup_file_watch(parent);
    open_file(path);
}

void ViewerWindow::present() {
    m_window.present();
}

// ── Handlers ────────────────────────────────────────────────────────

void ViewerWindow::handle_file_activated(const std::string& path) {
    open_file(path);
}

void ViewerWindow::handle_mode_changed(uint8_t mode) {
    // Keep header + canvas locked in step. set_mode on the canvas also
    // re-parses the cached document if one is loaded so directive
    // visibility flips immediately.
    m_header.set_mode(mode);
    m_canvas.set_mode(mode);

    // Swap the sidebar root to the mode-specific subfolder (tech/ vs
    // cms/) if such a sibling exists - lets documents that live only
    // under one branch disappear from the tree when the user toggles.
    switch_mode_directory(mode);

    if (m_canvas.has_document()) {
        update_status_bar_for_current();
    }
}

void ViewerWindow::handle_search_changed(const std::string& lowered) {
    m_tree_view.set_search(lowered);
}

void ViewerWindow::handle_mark_all_read() {
    m_tree_view.mark_all_read_display();

    const std::string& root = m_tree_view.root_path();
    if (root.empty()) return;

    fs::walk_markdown_files(root, [this](const std::string& path) {
        m_read_tracker.mark_read(path);
    });
    m_read_tracker.save(m_config_dir);
}

void ViewerWindow::handle_open_settings() {
    show_preferences_window(
        *m_window.native(),
        m_settings,
        m_config_dir,
        sigc::slot<void()>([this]() { m_canvas.queue_draw(); }));
}

void ViewerWindow::handle_refresh() {
    m_tree_view.refresh();
    if (!m_current_file.empty() && fs::path_exists(m_current_file)) {
        open_file(m_current_file);
    }
}

void ViewerWindow::handle_escape() {
    m_header.clear_search();
    m_tree_view.grab_focus();
}

void ViewerWindow::handle_focus_search() {
    m_header.focus_search();
}

void ViewerWindow::handle_copy_dump() {
    if (!m_canvas.has_document()) return;
    const std::string dump = m_canvas.dump_text();
    if (dump.empty()) return;

    // Route through the window widget's clipboard so the path matches
    // how the ase-gtk adapter already plugs into GDK. No manual Display
    // lookup needed.
    auto* widget = m_window.native_widget();
    if (widget == nullptr) return;
    auto clipboard = widget->get_clipboard();
    if (!clipboard) return;
    clipboard->set_text(dump);
}

// ── Internals ───────────────────────────────────────────────────────

void ViewerWindow::open_file(const std::string& path) {
    std::string content;
    if (!fs::read_file_to_string(path, content)) return;

    m_current_file = path;
    update_window_title(fs::base_name(path));

    if (m_read_tracker.mark_read(path)) {
        m_read_tracker.save(m_config_dir);
        m_tree_view.mark_file_read(path);
    }

    // Mode is an explicit UI decision: whatever the header toggle shows
    // is THE mode. No path sniffing, no content sniffing, no fallback -
    // those silently swallow bugs (render_generic panel instead of
    // directive output) and hide the real cause from the user.
    const uint8_t mode = m_canvas.current_mode();

    m_canvas.load_document(content, mode);
    m_status_bar.set_text(compute_status_text(path, content, mode));
    m_status_bar.set_progress(0);
}

void ViewerWindow::switch_mode_directory(uint8_t target_mode) {
    const std::string root = m_tree_view.root_path();
    if (root.empty()) return;

    std::string base = root;
    const std::string last = fs::base_name(base);
    if (last == "tech" || last == "cms") base = fs::parent_dir(base);
    const std::string mode_dir =
        fs::path_join(base, target_mode == 0 ? "tech" : "cms");
    if (fs::is_directory(mode_dir)) {
        m_tree_view.load(mode_dir);
        setup_file_watch(mode_dir);
    }
}

void ViewerWindow::update_status_bar_for_current() {
    if (!m_canvas.has_document() || m_current_file.empty()) {
        m_status_bar.set_text("No document");
        m_status_bar.set_progress(0);
        return;
    }
    m_status_bar.set_text(compute_status_text(
        m_current_file, m_canvas.content(), m_canvas.current_mode()));
    m_status_bar.set_progress(0);
}

void ViewerWindow::resolve_config_dir() {
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg != nullptr) {
        m_config_dir = std::string{xdg} + "/ase-viewer";
    } else {
        const char* home = std::getenv("HOME");
        m_config_dir = (home != nullptr ? std::string{home} : std::string{"/tmp"})
                     + "/.config/ase-viewer";
    }
    fs::make_directories(m_config_dir);
}

void ViewerWindow::setup_file_watch(const std::string& root) {
    m_file_monitor.watch(root, [this]() { handle_refresh(); });
}

void ViewerWindow::update_window_title(const std::string& subtitle) {
    m_window.set_title("ASE Viewer - " + subtitle);
}

}  // namespace ase::viewer
