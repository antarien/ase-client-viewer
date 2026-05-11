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

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gio/gio.h>
#include <glib.h>

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

ViewerWindow::ViewerWindow(ase::adp::gtk::ApplicationWindow window)
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

    // Push the persisted font size into the render pipeline before the
    // first paint. Without this the FONT_PX_* defaults (Tailwind body=12)
    // would silently override whatever the user previously set in
    // Preferences — the bug that made the font-size spin button look
    // dead until 2026-05-11.
    m_canvas.apply_font_size(m_settings.font_size);

    // ── Header bar ──
    m_window.set_titlebar(m_header);

    m_header.on_mode_changed([this](uint8_t mode) {
        handle_mode_changed(mode);
    });
    m_header.on_search_text([this](const std::string& lowered) {
        handle_search_changed(lowered);
    });
    m_header.on_search_toggle([this]() {
        handle_search_toggle();
    });
    m_header.on_mark_all_read([this]() {
        handle_mark_all_read();
    });
    m_header.on_refresh([this]() {
        handle_refresh();
    });
    m_header.on_settings([this]() {
        handle_open_settings();
    });

    // ── Sidebar header (pin + close-X) ───────────────────────────────
    m_pin_button.set_icon_name("view-pin-symbolic");
    m_pin_button.set_has_frame(false);
    m_pin_button.set_tooltip_text("Pin / unpin sidebar");
    m_pin_button.signal_clicked().connect(
        sigc::mem_fun(*this, &ViewerWindow::handle_pin_toggle));

    m_close_button.set_icon_name("window-close-symbolic");
    m_close_button.set_has_frame(false);
    m_close_button.set_tooltip_text("Close sidebar");
    m_close_button.signal_clicked().connect(
        sigc::mem_fun(*this, &ViewerWindow::handle_close_drawer));

    auto* spacer = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 0);
    spacer->set_hexpand(true);
    m_sidebar_header.append(*spacer);
    m_sidebar_header.append(m_pin_button);
    m_sidebar_header.append(m_close_button);

    m_sidebar_container.set_vexpand(true);
    m_sidebar_container.append(m_sidebar_header);
    m_sidebar_container.append(m_tree_view.widget());
    m_tree_view.widget().set_vexpand(true);

    m_tree_view.on_file_activated([this](const std::string& path) {
        handle_file_activated(path);
    });

    // ── Paned (sidebar slot + canvas) ────────────────────────────────
    m_paned.set_shrink_start_child(false);
    m_paned.set_vexpand(true);
    m_paned.set_start_child(m_sidebar_container);
    m_paned.set_end_child(m_canvas.widget());
    m_paned.set_position(m_settings.sidebar_width > 0 ? m_settings.sidebar_width : 280);

    g_signal_connect(m_paned.gobj(), "notify::position",
        G_CALLBACK(+[](GObject*, GParamSpec*, gpointer self) {
            static_cast<ViewerWindow*>(self)->handle_paned_position_changed();
        }), this);

    // ── Rail strip (collapsed sidebar) ───────────────────────────────
    m_rail.set_size_request(40, -1);
    m_rail.set_vexpand(true);
    m_rail.add_css_class("ase-rail");

    m_rail_open_button.set_icon_name("folder-symbolic");
    m_rail_open_button.set_has_frame(false);
    m_rail_open_button.set_tooltip_text("Open sidebar");
    m_rail_open_button.set_margin_top(8);
    m_rail_open_button.signal_clicked().connect(
        sigc::mem_fun(*this, &ViewerWindow::handle_open_drawer));
    m_rail.append(m_rail_open_button);

    GtkGesture* rail_drag = gtk_gesture_drag_new();
    g_signal_connect(rail_drag, "drag-update",
        G_CALLBACK(+[](GtkGestureDrag*, double offset_x, double, gpointer self) {
            if (offset_x < 8.0) return;
            auto* w = static_cast<ViewerWindow*>(self);
            int width = static_cast<int>(40.0 + offset_x);
            if (width < 180) width = 180;
            if (width > 600) width = 600;
            w->m_settings.sidebar_width = width;
            w->m_settings.sidebar_open  = true;
            w->apply_sidebar_state();
        }), this);
    gtk_widget_add_controller(GTK_WIDGET(m_rail.gobj()), GTK_EVENT_CONTROLLER(rail_drag));

    // ── Content box (rail + paned), wrapped in overlay ───────────────
    m_content_box.set_hexpand(true);
    m_content_box.set_vexpand(true);
    m_content_box.append(m_rail);
    m_content_box.append(m_paned);
    m_paned.set_hexpand(true);

    m_overlay.set_child(m_content_box);

    // Backdrop overlay child
    m_backdrop.set_hexpand(true);
    m_backdrop.set_vexpand(true);
    m_backdrop.add_css_class("ase-drawer-backdrop");
    {
        GtkGesture* tap = gtk_gesture_click_new();
        g_signal_connect(tap, "pressed",
            G_CALLBACK(+[](GtkGestureClick*, int, double, double, gpointer self) {
                static_cast<ViewerWindow*>(self)->handle_close_drawer();
            }), this);
        gtk_widget_add_controller(GTK_WIDGET(m_backdrop.gobj()),
                                  GTK_EVENT_CONTROLLER(tap));
    }
    m_overlay.add_overlay(m_backdrop);

    m_overlay_sidebar_holder.set_halign(Gtk::Align::START);
    m_overlay_sidebar_holder.set_valign(Gtk::Align::FILL);
    m_overlay_sidebar_holder.set_vexpand(true);
    m_overlay_sidebar_holder.add_css_class("ase-drawer-floating");
    m_overlay.add_overlay(m_overlay_sidebar_holder);

    m_backdrop.set_visible(false);
    m_overlay_sidebar_holder.set_visible(false);

    // ── Main vertical layout: overlay + status bar ───────────────────
    m_main_box.append(m_overlay);
    m_main_box.append(m_status_bar.widget());
    m_window.native()->set_child(m_main_box);

    apply_sidebar_state();

    // ── Keyboard shortcuts ──
    m_shortcuts.on_focus_search([this]() { handle_focus_search(); });
    m_shortcuts.on_escape      ([this]() { handle_escape(); });
    m_shortcuts.on_refresh     ([this]() { handle_refresh(); });
    m_shortcuts.on_settings    ([this]() { handle_open_settings(); });
    m_shortcuts.on_mode([this](uint8_t mode) { handle_mode_changed(mode); });
    m_shortcuts.on_copy_dump   ([this]() { handle_copy_dump(); });
    m_shortcuts.install_on(*m_window.native_widget());

    // ── Type-ahead search ──
    // Any printable keystroke (no modifiers) opens the search bar and
    // seeds the entry with the typed character. Mirrors the
    // ase-client-explorer type-ahead handler: CAPTURE phase so the
    // tree view's own keyboard controller doesn't consume the key first.
    {
        GtkEventController* type_ahead = gtk_event_controller_key_new();
        gtk_event_controller_set_propagation_phase(type_ahead, GTK_PHASE_CAPTURE);
        g_signal_connect(type_ahead, "key-pressed",
            G_CALLBACK(+[](GtkEventControllerKey*, guint keyval, guint,
                           GdkModifierType state, gpointer self) -> gboolean {
                auto* w = static_cast<ViewerWindow*>(self);
                return w->handle_type_ahead(keyval, static_cast<unsigned>(state)) ? TRUE : FALSE;
            }), this);
        gtk_widget_add_controller(
            GTK_WIDGET(m_window.native()->gobj()), type_ahead);
    }

    // ── Drop target ──
    // Files / folders dragged from a file manager land here and open
    // directly inside the viewer. Single-item drop only; for a directory
    // we replace the tree root, for a regular file we open it.
    setup_drop_target();
}

void ViewerWindow::setup_drop_target() {
    GtkDropTarget* drop = gtk_drop_target_new(G_TYPE_FILE, GDK_ACTION_COPY);
    g_signal_connect(drop, "drop",
        G_CALLBACK(+[](GtkDropTarget*, const GValue* value, double, double,
                       gpointer self) -> gboolean {
            if (value == nullptr || !G_VALUE_HOLDS(value, G_TYPE_FILE)) {
                return FALSE;
            }
            GFile* gfile = G_FILE(g_value_get_object(value));
            if (gfile == nullptr) return FALSE;
            char* path_c = g_file_get_path(gfile);
            if (path_c == nullptr) return FALSE;
            std::string path{path_c};
            g_free(path_c);

            auto* w = static_cast<ViewerWindow*>(self);
            if (fs::is_directory(path)) {
                w->load_directory(path);
                return TRUE;
            }
            if (fs::is_regular_file(path)) {
                w->load_file(path);
                return TRUE;
            }
            return FALSE;
        }), this);
    gtk_widget_add_controller(
        GTK_WIDGET(m_window.native()->gobj()), GTK_EVENT_CONTROLLER(drop));
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
    if (m_settings.sidebar_open && !m_settings.sidebar_pinned) {
        handle_close_drawer();
    }
}

void ViewerWindow::apply_sidebar_state() {
    m_applying_sidebar_state = true;

    const bool open   = m_settings.sidebar_open;
    const bool pinned = m_settings.sidebar_pinned;
    const int  width  = m_settings.sidebar_width > 0 ? m_settings.sidebar_width : 280;

    auto* sb_parent = m_sidebar_container.get_parent();
    if (sb_parent == &m_overlay_sidebar_holder) {
        m_overlay_sidebar_holder.remove(m_sidebar_container);
    } else if (sb_parent != nullptr) {
        gtk_paned_set_start_child(GTK_PANED(m_paned.gobj()), nullptr);
    }

    m_close_button.set_visible(open && !pinned);

    if (!open) {
        m_rail.set_visible(true);
        m_paned.set_position(0);
        gtk_paned_set_start_child(GTK_PANED(m_paned.gobj()), nullptr);
        m_backdrop.set_visible(false);
        m_overlay_sidebar_holder.set_visible(false);
    } else if (pinned) {
        m_rail.set_visible(false);
        m_paned.set_start_child(m_sidebar_container);
        m_paned.set_position(width);
        m_backdrop.set_visible(false);
        m_overlay_sidebar_holder.set_visible(false);
    } else {
        m_rail.set_visible(false);
        m_paned.set_position(0);
        gtk_paned_set_start_child(GTK_PANED(m_paned.gobj()), nullptr);
        m_overlay_sidebar_holder.append(m_sidebar_container);
        m_overlay_sidebar_holder.set_size_request(width, -1);
        m_overlay_sidebar_holder.set_margin_start(0);
        m_backdrop.set_visible(true);
        m_overlay_sidebar_holder.set_visible(true);
    }

    save_settings(m_config_dir, m_settings);
    m_applying_sidebar_state = false;
}

void ViewerWindow::handle_pin_toggle() {
    if (!m_settings.sidebar_pinned) {
        m_settings.sidebar_pinned = true;
        m_settings.sidebar_open   = true;
    } else {
        m_settings.sidebar_pinned = false;
    }
    apply_sidebar_state();
}

void ViewerWindow::handle_close_drawer() {
    m_settings.sidebar_open = false;
    apply_sidebar_state();
}

void ViewerWindow::handle_open_drawer() {
    m_settings.sidebar_open = true;
    apply_sidebar_state();
}

void ViewerWindow::handle_paned_position_changed() {
    if (m_applying_sidebar_state) return;
    if (!m_settings.sidebar_open) return;
    const int pos = m_paned.get_position();
    if (pos > 0 && pos < 60) {
        handle_close_drawer();
        return;
    }
    if (pos >= 60 && m_settings.sidebar_pinned) {
        m_settings.sidebar_width = pos;
        save_settings(m_config_dir, m_settings);
    }
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

void ViewerWindow::handle_search_toggle() {
    // Magnifier button click / Ctrl+F / Escape from inside the entry all
    // funnel through here. Toggle visibility and reset the tree-view
    // filter when closing so the user doesn't get "stuck" in a filtered
    // state when they dismiss the bar.
    const bool show = !m_header.search_visible();
    m_header.toggle_search(show);
    if (!show) {
        m_tree_view.set_search("");
    }
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
    // Capture the default_root the prefs window opened with. Every
    // setting-change inside the prefs window fires on_changed; if the
    // root has been swapped (typically via the Browse… button) we reload
    // the tree so the user sees their new docs root immediately —
    // otherwise the change only takes effect on the next viewer launch.
    const std::string root_before = m_settings.default_root;
    show_preferences_window(
        *m_window.native(),
        m_settings,
        m_config_dir,
        sigc::slot<void()>([this, root_before]() {
            m_canvas.apply_font_size(m_settings.font_size);
            const std::string& root_after = m_settings.default_root;
            if (root_after != root_before
                && !root_after.empty()
                && fs::is_directory(root_after)) {
                load_directory(root_after);
            }
        }));
}

void ViewerWindow::handle_refresh() {
    m_tree_view.refresh();
    if (!m_current_file.empty() && fs::path_exists(m_current_file)) {
        open_file(m_current_file);
    }
}

void ViewerWindow::handle_escape() {
    // Three-tier Escape: search bar first, then the unpinned drawer,
    // finally the window itself.
    if (m_header.search_visible()) {
        m_header.toggle_search(false);
        m_tree_view.set_search("");
        m_tree_view.grab_focus();
        return;
    }
    if (m_settings.sidebar_open && !m_settings.sidebar_pinned) {
        handle_close_drawer();
        return;
    }
    gtk_window_close(GTK_WINDOW(m_window.native()->gobj()));
}

void ViewerWindow::handle_focus_search() {
    // Ctrl+F: open the search bar if closed, otherwise re-focus the
    // entry so the user can keep typing.
    if (!m_header.search_visible()) {
        m_header.toggle_search(true);
    } else {
        m_header.focus_search();
    }
}

bool ViewerWindow::handle_type_ahead(unsigned keyval, unsigned state) {
    // Already in search mode → let the entry handle the key normally.
    if (m_header.search_visible()) return false;

    // Skip when any modifier is held so Ctrl+F / F5 / Alt+… keep
    // routing to their own shortcut handlers untouched.
    constexpr unsigned MODIFIERS =
        static_cast<unsigned>(GDK_CONTROL_MASK) |
        static_cast<unsigned>(GDK_ALT_MASK)     |
        static_cast<unsigned>(GDK_SUPER_MASK)   |
        static_cast<unsigned>(GDK_META_MASK);
    if (state & MODIFIERS) return false;

    // Non-printable keys (arrows, Enter, Tab, F-keys, Escape …) convert
    // to 0 or a control character — those fall through.
    const gunichar uc = gdk_keyval_to_unicode(keyval);
    if (uc == 0 || g_unichar_iscntrl(uc)) return false;

    // Open the search bar and seed it with the typed character.
    m_header.toggle_search(true);

    char utf8[8] = {0};
    const int len = g_unichar_to_utf8(uc, utf8);
    utf8[len] = '\0';
    m_header.seed_search_with(std::string{utf8});

    return true;
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
