/**
 * ASE TECH & DESIGN Viewer — GTK4 Desktop Application
 *
 * Entry point. Handles CLI argument parsing and application lifecycle.
 *
 * Start modes:
 *   ase-viewer /path/to/docs/   → Directory mode (sidebar + content)
 *   ase-viewer document.md      → Single file mode
 *   ase-viewer                  → Welcome screen with directory chooser
 */

#include <gtkmm.h>
#include <giomm.h>
#include <viewer/engine.hpp>
#include <ase/markdown/markdown.hpp>
#include "render/vwr_rndr_doc.hpp"
#include "vwr_settings.hpp"
#include "vwr_preferences.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <set>
#include <cstdlib>
#include <gdk/gdkkeysyms.h>

namespace fs = std::filesystem;

// ── Dark Theme CSS ──────────────────────────────────────────────────

static const char* CSS_DARK = R"(
    window {
        background-color: #040404;
        color: #8A9A9A;
    }
    headerbar {
        background: linear-gradient(to right, #0A0A0A, #121212);
        border-bottom: 1px solid rgba(255, 255, 255, 0.03);
        color: #5a9cb8;
        min-height: 32px;
    }
    .sidebar {
        background-color: #040404;
        border-right: 1px solid rgba(255, 255, 255, 0.03);
    }
    .sidebar treeview {
        background-color: transparent;
        color: #5A5A5A;
    }
    .sidebar treeview:selected {
        background-color: #121212;
        color: #5a9cb8;
    }
    .content-area {
        background-color: #040404;
    }
    .mode-button {
        min-height: 20px;
        min-width: 48px;
        padding: 2px 8px;
        font-size: 10px;
        font-family: 'Fira Code', monospace;
    }
    .mode-button:checked {
        background-color: #5a9cb8;
        color: #040404;
    }
    .welcome-title {
        font-size: 14px;
        font-family: 'Fira Code', monospace;
        color: #5a9cb8;
    }
    .welcome-subtitle {
        font-size: 11px;
        font-family: 'Fira Code', monospace;
        color: #5A5A5A;
    }
    .search-entry {
        background-color: #0A0A0A;
        color: #8A9A9A;
        border: 1px solid rgba(255, 255, 255, 0.03);
        font-family: 'Fira Code', monospace;
        font-size: 11px;
        min-height: 24px;
    }
    .search-entry:focus {
        border-color: #5a9cb8;
    }
    .sidebar treeview row:hover {
        background-color: rgba(90, 156, 184, 0.06);
    }
    .status-bar {
        background-color: #0A0A0A;
        border-top: 1px solid rgba(255, 255, 255, 0.03);
        padding: 2px 8px;
        font-family: 'Fira Code', monospace;
        font-size: 9px;
        color: #5A5A5A;
        min-height: 18px;
    }
    progressbar trough {
        background-color: #121212;
        min-height: 4px;
    }
    progressbar progress {
        background-color: #5a9cb8;
        min-height: 4px;
    }
)";

// ── File Tree Model ─────────────────────────────────────────────────

class FileTreeColumns : public Gtk::TreeModelColumnRecord {
public:
    FileTreeColumns() { add(col_display); add(col_name); add(col_path); add(col_is_dir); add(col_is_read); }
    Gtk::TreeModelColumn<Glib::ustring> col_display; // rendered text (name + badges)
    Gtk::TreeModelColumn<Glib::ustring> col_name;    // raw filename
    Gtk::TreeModelColumn<std::string> col_path;
    Gtk::TreeModelColumn<bool> col_is_dir;
    Gtk::TreeModelColumn<bool> col_is_read;
};

// ── ViewerWindow ────────────────────────────────────────────────────

class ViewerWindow : public Gtk::ApplicationWindow {
public:
    ViewerWindow() {
        set_title("ASE TECH & DESIGN Viewer");
        set_default_size(1400, 900);
        build_ui();
    }

    void load_directory(const std::string& path) {
        m_root_path = path;
        populate_tree(path);
        setup_file_monitor(path);
        set_title("ASE Viewer — " + path);
    }

    void load_file(const std::string& path) {
        m_root_path = fs::path(path).parent_path().string();
        populate_tree(m_root_path);
        setup_file_monitor(m_root_path);
        open_file(path);
        set_title("ASE Viewer — " + fs::path(path).filename().string());
    }

private:
    std::string m_root_path;
    FileTreeColumns m_columns;
    Glib::RefPtr<Gtk::TreeStore> m_tree_store;
    Glib::RefPtr<Gtk::TreeModelFilter> m_filter_model;
    Gtk::TreeView m_tree_view;
    Gtk::DrawingArea m_canvas;
    Gtk::Paned m_paned{Gtk::Orientation::HORIZONTAL};
    Gtk::SearchEntry m_search;
    Gtk::ProgressBar m_progress;
    Gtk::Label m_status_label;
    std::string m_search_text;

    // File watching
    Glib::RefPtr<Gio::FileMonitor> m_file_monitor;
    sigc::connection m_reload_timer;

    // Read tracking + settings
    std::set<std::string> m_read_files;
    std::string m_config_dir;
    ase::viewer::ViewerSettings m_settings;

    // Current document
    std::string m_content;
    std::string m_current_file;
    ase::markdown::Document m_doc{};
    bool m_has_doc = false;
    uint8_t m_mode = 0; // 0=TECH, 1=DSGN

    void build_ui() {
        // XDG config directory
        const char* xdg = std::getenv("XDG_CONFIG_HOME");
        m_config_dir = xdg ? std::string(xdg) + "/ase-viewer"
                           : std::string(std::getenv("HOME")) + "/.config/ase-viewer";
        fs::create_directories(m_config_dir);
        load_read_state();
        m_settings = ase::viewer::load_settings(m_config_dir);
        m_mode = m_settings.default_mode;

        // HeaderBar
        auto header = Gtk::make_managed<Gtk::HeaderBar>();
        set_titlebar(*header);

        // Title
        auto title = Gtk::make_managed<Gtk::Label>("ASE TECH & DESIGN Viewer");
        title->add_css_class("title");
        header->set_title_widget(*title);

        // Mode toggle buttons
        auto mode_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 0);
        auto btn_tech = Gtk::make_managed<Gtk::ToggleButton>("TECH");
        auto btn_dsgn = Gtk::make_managed<Gtk::ToggleButton>("DSGN");
        btn_tech->set_active(true);
        btn_tech->add_css_class("mode-button");
        btn_dsgn->add_css_class("mode-button");
        btn_tech->set_group(*btn_dsgn);
        mode_box->append(*btn_tech);
        mode_box->append(*btn_dsgn);
        header->pack_start(*mode_box);

        btn_tech->signal_toggled().connect([this, btn_tech]() {
            m_mode = btn_tech->get_active() ? 0 : 1;
            m_canvas.queue_draw();
        });

        // Search
        m_search.set_placeholder_text("Search docs...  (Ctrl+F)");
        m_search.add_css_class("search-entry");
        m_search.signal_search_changed().connect([this]() {
            auto raw = m_search.get_text();
            m_search_text.clear();
            for (auto ch : raw) m_search_text += static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            if (m_filter_model) m_filter_model->refilter();
            if (!m_search_text.empty()) expand_matching_rows();
        });
        header->pack_end(m_search);

        // Mark-All-Read button
        auto btn_mark_read = Gtk::make_managed<Gtk::Button>("\xe2\x9c\x93"); // ✓
        btn_mark_read->set_tooltip_text("Mark all as read");
        btn_mark_read->add_css_class("mode-button");
        btn_mark_read->signal_clicked().connect([this]() {
            mark_all_read();
        });
        header->pack_end(*btn_mark_read);

        // Settings button (gear icon)
        auto btn_settings = Gtk::make_managed<Gtk::Button>("\xe2\x9a\x99"); // ⚙
        btn_settings->set_tooltip_text("Settings (Ctrl+,)");
        btn_settings->add_css_class("mode-button");
        btn_settings->signal_clicked().connect([this]() {
            open_settings();
        });
        header->pack_end(*btn_settings);

        // Main layout: VBox → Paned + StatusBar
        auto main_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 0);
        m_paned.set_position(280);
        m_paned.set_shrink_start_child(false);
        m_paned.set_vexpand(true);
        main_box->append(m_paned);

        // Status bar
        auto status_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
        status_box->add_css_class("status-bar");
        m_status_label.set_text("No document");
        m_status_label.set_hexpand(true);
        m_status_label.set_xalign(0);
        status_box->append(m_status_label);
        m_progress.set_fraction(0);
        m_progress.set_valign(Gtk::Align::CENTER);
        m_progress.set_size_request(120, 4);
        status_box->append(m_progress);
        main_box->append(*status_box);

        set_child(*main_box);

        // Sidebar
        auto sidebar_scroll = Gtk::make_managed<Gtk::ScrolledWindow>();
        sidebar_scroll->add_css_class("sidebar");
        sidebar_scroll->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);

        m_tree_store = Gtk::TreeStore::create(m_columns);
        m_filter_model = Gtk::TreeModelFilter::create(m_tree_store);
        m_filter_model->set_visible_func([this](const Gtk::TreeModel::const_iterator& iter) -> bool {
            return filter_row(iter);
        });
        m_tree_view.set_model(m_filter_model);
        m_tree_view.set_headers_visible(false);
        auto* col = Gtk::make_managed<Gtk::TreeViewColumn>("");
        auto* cell = Gtk::make_managed<Gtk::CellRendererText>();
        col->pack_start(*cell, true);
        col->add_attribute(cell->property_markup(), m_columns.col_display);
        m_tree_view.append_column(*col);
        m_tree_view.set_activate_on_single_click(true);

        m_tree_view.signal_row_activated().connect(
            [this](const Gtk::TreeModel::Path& filter_path, Gtk::TreeViewColumn*) {
                auto iter = m_filter_model->get_iter(filter_path);
                if (!iter) return;
                bool is_dir = (*iter)[m_columns.col_is_dir];
                if (is_dir) {
                    if (m_tree_view.row_expanded(filter_path))
                        m_tree_view.collapse_row(filter_path);
                    else
                        m_tree_view.expand_row(filter_path, false);
                } else {
                    std::string file_path = static_cast<std::string>((*iter)[m_columns.col_path]);
                    open_file(file_path);
                }
            });

        sidebar_scroll->set_child(m_tree_view);
        m_paned.set_start_child(*sidebar_scroll);

        // Content area: DrawingArea in ScrolledWindow
        auto content_scroll = Gtk::make_managed<Gtk::ScrolledWindow>();
        content_scroll->add_css_class("content-area");
        content_scroll->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);

        m_canvas.set_draw_func(sigc::mem_fun(*this, &ViewerWindow::on_draw));
        m_canvas.set_hexpand(true);
        m_canvas.set_vexpand(true);

        content_scroll->set_child(m_canvas);
        m_paned.set_end_child(*content_scroll);

        // Keyboard shortcuts
        auto key_ctrl = Gtk::EventControllerKey::create();
        key_ctrl->signal_key_pressed().connect(
            [this](guint keyval, guint, Gdk::ModifierType state) -> bool {
                bool ctrl = (state & Gdk::ModifierType::CONTROL_MASK) != Gdk::ModifierType{};
                if (ctrl && keyval == GDK_KEY_f) {
                    m_search.grab_focus();
                    return true;
                }
                if (keyval == GDK_KEY_Escape) {
                    m_search.set_text("");
                    m_tree_view.grab_focus();
                    return true;
                }
                if (keyval == GDK_KEY_F5) {
                    refresh_tree();
                    return true;
                }
                if (ctrl && keyval == GDK_KEY_comma) {
                    open_settings();
                    return true;
                }
                return false;
            }, false);
        add_controller(key_ctrl);
    }

    // ── Search filter ───────────────────────────────────────────────

    bool filter_row(const Gtk::TreeModel::const_iterator& iter) const {
        if (m_search_text.empty()) return true;
        // Directories visible if any child matches
        bool is_dir = (*iter)[m_columns.col_is_dir];
        Glib::ustring name = (*iter)[m_columns.col_name];
        std::string lower;
        for (auto ch : name) lower += static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        if (lower.find(m_search_text) != std::string::npos) return true;
        if (is_dir) {
            auto children = iter->children();
            for (auto child = children.begin(); child != children.end(); ++child) {
                if (filter_row(child)) return true;
            }
        }
        return false;
    }

    void expand_matching_rows() {
        m_filter_model->foreach_iter([this](const Gtk::TreeModel::const_iterator& iter) -> bool {
            bool is_dir = (*iter)[m_columns.col_is_dir];
            if (is_dir) {
                auto path = m_filter_model->get_path(iter);
                m_tree_view.expand_row(path, false);
            }
            return false; // continue
        });
    }

    // ── Read tracking ─────────────────────────────────────────────

    static Glib::ustring format_display(const std::string& name, bool is_dir, bool is_read) {
        // Pango markup: unread files are bright, read files are dimmed
        if (is_dir) {
            return Glib::ustring::compose("<span foreground='#5A5A5A'>%1</span>",
                Glib::Markup::escape_text(name));
        }
        if (is_read) {
            return Glib::ustring::compose("<span foreground='#3A3A3A'>%1</span>",
                Glib::Markup::escape_text(name));
        }
        // Unread: bright cyan dot + white text
        return Glib::ustring::compose(
            "<span foreground='#5a9cb8'>\xe2\x97\x8f </span><span foreground='#8A9A9A'>%1</span>",
            Glib::Markup::escape_text(name));
    }

    void update_tree_display(const std::string& file_path) {
        m_tree_store->foreach_iter([this, &file_path](const Gtk::TreeModel::iterator& iter) -> bool {
            std::string path = static_cast<std::string>((*iter)[m_columns.col_path]);
            if (path == file_path) {
                Glib::ustring name = (*iter)[m_columns.col_name];
                (*iter)[m_columns.col_is_read] = true;
                (*iter)[m_columns.col_display] = format_display(name.raw(), false, true);
                return true; // stop
            }
            return false;
        });
    }

    void mark_all_read() {
        m_tree_store->foreach_iter([this](const Gtk::TreeModel::iterator& iter) -> bool {
            bool is_dir = (*iter)[m_columns.col_is_dir];
            if (!is_dir) {
                std::string path = static_cast<std::string>((*iter)[m_columns.col_path]);
                m_read_files.insert(path);
                Glib::ustring name = (*iter)[m_columns.col_name];
                (*iter)[m_columns.col_is_read] = true;
                (*iter)[m_columns.col_display] = format_display(name.raw(), false, true);
            }
            return false;
        });
        save_read_state();
    }

    void load_read_state() {
        auto path = m_config_dir + "/read-files.txt";
        std::ifstream f(path);
        if (!f.is_open()) return;
        std::string line;
        while (std::getline(f, line)) {
            if (!line.empty()) m_read_files.insert(line);
        }
    }

    void save_read_state() {
        auto path = m_config_dir + "/read-files.txt";
        std::ofstream f(path);
        for (const auto& p : m_read_files) {
            f << p << '\n';
        }
    }

    void open_settings() {
        ase::viewer::show_preferences_window(*this, m_settings, m_config_dir, [this]() {
            m_canvas.queue_draw();
        });
    }

    // ── File watching ───────────────────────────────────────────────

    void setup_file_monitor(const std::string& root) {
        if (m_file_monitor) m_file_monitor->cancel();
        auto dir = Gio::File::create_for_path(root);
        m_file_monitor = dir->monitor_directory(Gio::FileMonitor::Flags::WATCH_MOVES);
        m_file_monitor->signal_changed().connect(
            [this](const Glib::RefPtr<Gio::File>&,
                   const Glib::RefPtr<Gio::File>&,
                   Gio::FileMonitor::Event) {
                // Debounce: 500ms delay to batch rapid changes
                if (m_reload_timer.connected()) m_reload_timer.disconnect();
                m_reload_timer = Glib::signal_timeout().connect([this]() -> bool {
                    refresh_tree();
                    return false; // one-shot
                }, 500);
            });
    }

    void refresh_tree() {
        if (m_root_path.empty()) return;
        // Save expanded state
        std::vector<std::string> expanded_paths;
        m_filter_model->foreach_iter([this, &expanded_paths](const Gtk::TreeModel::const_iterator& iter) -> bool {
            auto path = m_filter_model->get_path(iter);
            if (m_tree_view.row_expanded(path)) {
                std::string file_path = static_cast<std::string>((*iter)[m_columns.col_path]);
                expanded_paths.push_back(file_path);
            }
            return false;
        });
        // Repopulate
        populate_tree(m_root_path);
        // Restore expanded state
        m_filter_model->foreach_iter([this, &expanded_paths](const Gtk::TreeModel::const_iterator& iter) -> bool {
            std::string file_path = static_cast<std::string>((*iter)[m_columns.col_path]);
            for (const auto& ep : expanded_paths) {
                if (ep == file_path) {
                    auto path = m_filter_model->get_path(iter);
                    m_tree_view.expand_row(path, false);
                    break;
                }
            }
            return false;
        });
        // Re-render current file if it changed on disk
        if (!m_current_file.empty() && fs::exists(m_current_file)) {
            open_file(m_current_file);
        }
    }

    void populate_tree(const std::string& root) {
        m_tree_store->clear();
        if (!fs::exists(root) || !fs::is_directory(root)) return;
        populate_node(root, Gtk::TreeModel::iterator());
    }

    void populate_node(const std::string& dir_path, const Gtk::TreeModel::iterator& parent) {
        // Collect and sort entries
        struct Entry { std::string name; std::string path; bool is_dir; };
        std::vector<Entry> entries;

        for (const auto& entry : fs::directory_iterator(dir_path)) {
            auto name = entry.path().filename().string();
            if (name[0] == '.') continue;
            if (entry.is_directory()) {
                entries.push_back({name, entry.path().string(), true});
            } else if (name.size() > 3 && name.substr(name.size() - 3) == ".md") {
                entries.push_back({name, entry.path().string(), false});
            }
        }

        std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) {
            if (a.is_dir != b.is_dir) return a.is_dir > b.is_dir;
            return a.name < b.name;
        });

        for (const auto& e : entries) {
            Gtk::TreeModel::iterator iter;
            if (parent) iter = m_tree_store->append(parent->children());
            else iter = m_tree_store->append();
            (*iter)[m_columns.col_name] = e.name;
            (*iter)[m_columns.col_path] = e.path;
            (*iter)[m_columns.col_is_dir] = e.is_dir;
            bool is_read = e.is_dir || m_read_files.count(e.path) > 0;
            (*iter)[m_columns.col_is_read] = is_read;
            (*iter)[m_columns.col_display] = format_display(e.name, e.is_dir, is_read);
            if (e.is_dir) populate_node(e.path, iter);
        }
    }

    void open_file(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) return;
        std::ostringstream ss;
        ss << file.rdbuf();
        m_content = ss.str();
        m_current_file = path;
        set_title("ASE Viewer — " + fs::path(path).filename().string());

        // Mark as read
        if (m_read_files.insert(path).second) {
            save_read_state();
            update_tree_display(path);
        }

        // Parse markdown into AST
        if (m_has_doc) ase::markdown::free_document(m_doc);
        ase::markdown::ParseOptions opts{};
        opts.mode = m_mode;
        opts.parse_frontmatter = 1;
        m_doc = ase::markdown::parse(m_content.c_str(), static_cast<uint32_t>(m_content.size()), opts);
        m_has_doc = true;

        // Update status bar
        int word_count = 0;
        bool in_word = false;
        for (char c : m_content) {
            if (std::isalnum(static_cast<unsigned char>(c))) {
                if (!in_word) { word_count++; in_word = true; }
            } else {
                in_word = false;
            }
        }
        int read_min = (word_count + 199) / 200; // ~200 wpm
        std::string mode_str = m_mode == 0 ? "TECH" : "DSGN";
        m_status_label.set_text(
            fs::path(path).filename().string() + "  |  " +
            std::to_string(word_count) + " words  |  ~" +
            std::to_string(read_min) + " min read  |  " + mode_str);
        m_progress.set_fraction(0);

        m_canvas.queue_draw();
    }

    void on_draw(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height) {
        (void)height;

        if (!m_has_doc) {
            // Welcome screen
            cr->set_source_rgb(0.35, 0.61, 0.72);
            cr->select_font_face("Fira Code", Cairo::ToyFontFace::Slant::NORMAL, Cairo::ToyFontFace::Weight::BOLD);
            cr->set_font_size(14);
            cr->move_to(width / 2.0 - 120, 200);
            cr->show_text("ASE TECH & DESIGN Viewer");

            cr->set_source_rgb(0.35, 0.35, 0.35);
            cr->set_font_size(11);
            cr->move_to(width / 2.0 - 100, 230);
            cr->show_text("Select a document from the sidebar");
            return;
        }

        // Render AST via document renderer
        render::RenderContext ctx;
        ctx.cr = cr;
        ctx.width = width;
        ctx.viewport_h = height;
        double total_height = render::render_document(ctx, m_doc);
        m_canvas.set_content_height(static_cast<int>(total_height));
    }
};

// ── ViewerApp ───────────────────────────────────────────────────────

class ViewerApp : public Gtk::Application {
public:
    ViewerApp()
        : Gtk::Application("com.antarien.ase.viewer",
                           Gio::Application::Flags::HANDLES_OPEN) {}

protected:
    void on_startup() override {
        Gtk::Application::on_startup();

        // Load dark theme CSS
        auto css = Gtk::CssProvider::create();
        css->load_from_data(CSS_DARK);
        Gtk::StyleContext::add_provider_for_display(
            Gdk::Display::get_default(), css,
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

        // Prefer dark theme
        auto settings = Gtk::Settings::get_default();
        settings->property_gtk_application_prefer_dark_theme() = true;
    }

    void on_activate() override {
        auto window = create_window();
        window->present();
    }

    void on_open(const Gio::Application::type_vec_files& files,
                 const Glib::ustring& hint) override {
        (void)hint;
        auto window = create_window();

        if (!files.empty()) {
            auto path = files[0]->get_path();
            if (fs::is_directory(path)) {
                window->load_directory(path);
            } else if (fs::is_regular_file(path)) {
                window->load_file(path);
            }
        }

        window->present();
    }

private:
    ViewerWindow* create_window() {
        auto window = new ViewerWindow();
        add_window(*window);
        return window;
    }
};

int main(int argc, char* argv[]) {
    auto app = ViewerApp::create();
    return app->run(argc, argv);
}
