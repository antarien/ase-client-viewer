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
#include <viewer/engine.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>

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
)";

// ── File Tree Model ─────────────────────────────────────────────────

class FileTreeColumns : public Gtk::TreeModelColumnRecord {
public:
    FileTreeColumns() { add(col_name); add(col_path); add(col_is_dir); }
    Gtk::TreeModelColumn<Glib::ustring> col_name;
    Gtk::TreeModelColumn<std::string> col_path;
    Gtk::TreeModelColumn<bool> col_is_dir;
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
        set_title("ASE Viewer — " + path);
    }

    void load_file(const std::string& path) {
        m_root_path = fs::path(path).parent_path().string();
        populate_tree(m_root_path);
        open_file(path);
        set_title("ASE Viewer — " + fs::path(path).filename().string());
    }

private:
    std::string m_root_path;
    FileTreeColumns m_columns;
    Glib::RefPtr<Gtk::TreeStore> m_tree_store;
    Gtk::TreeView m_tree_view;
    Gtk::DrawingArea m_canvas;
    Gtk::Paned m_paned{Gtk::Orientation::HORIZONTAL};
    Gtk::SearchEntry m_search;

    // Current document content
    std::string m_content;
    uint8_t m_mode = 0; // 0=TECH, 1=DSGN

    void build_ui() {
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
        m_search.set_placeholder_text("Search docs...");
        m_search.add_css_class("search-entry");
        header->pack_end(m_search);

        // Main layout: Paned
        m_paned.set_position(280);
        m_paned.set_shrink_start_child(false);
        set_child(m_paned);

        // Sidebar
        auto sidebar_scroll = Gtk::make_managed<Gtk::ScrolledWindow>();
        sidebar_scroll->add_css_class("sidebar");
        sidebar_scroll->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);

        m_tree_store = Gtk::TreeStore::create(m_columns);
        m_tree_view.set_model(m_tree_store);
        m_tree_view.set_headers_visible(false);
        m_tree_view.append_column("", m_columns.col_name);
        m_tree_view.set_activate_on_single_click(true);

        m_tree_view.signal_row_activated().connect(
            [this](const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn*) {
                auto iter = m_tree_store->get_iter(path);
                if (!iter) return;
                bool is_dir = (*iter)[m_columns.col_is_dir];
                if (is_dir) {
                    if (m_tree_view.row_expanded(path))
                        m_tree_view.collapse_row(path);
                    else
                        m_tree_view.expand_row(path, false);
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
            if (e.is_dir) populate_node(e.path, iter);
        }
    }

    void open_file(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) return;
        std::ostringstream ss;
        ss << file.rdbuf();
        m_content = ss.str();
        set_title("ASE Viewer — " + fs::path(path).filename().string());

        // Trigger redraw with new content
        m_canvas.set_content_height(static_cast<int>(m_content.size() / 2));
        m_canvas.queue_draw();
    }

    void on_draw(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height) {
        (void)height;

        if (m_content.empty()) {
            // Welcome screen
            cr->set_source_rgb(0.35, 0.61, 0.72); // PANEL_CYAN
            cr->select_font_face("Fira Code", Cairo::ToyFontFace::Slant::NORMAL, Cairo::ToyFontFace::Weight::BOLD);
            cr->set_font_size(14);
            cr->move_to(width / 2.0 - 120, 200);
            cr->show_text("ASE TECH & DESIGN Viewer");

            cr->set_source_rgb(0.35, 0.35, 0.35); // TEXT_LABEL
            cr->set_font_size(11);
            cr->move_to(width / 2.0 - 100, 230);
            cr->show_text("Select a document from the sidebar");
            return;
        }

        // Placeholder: render raw text (proper AST rendering in Phase 4)
        cr->set_source_rgb(0.54, 0.60, 0.60); // TEXT_PRIMARY
        cr->select_font_face("Fira Code", Cairo::ToyFontFace::Slant::NORMAL, Cairo::ToyFontFace::Weight::NORMAL);
        cr->set_font_size(11);

        double y = 24;
        double line_height = 16;
        std::istringstream stream(m_content);
        std::string line;
        while (std::getline(stream, line)) {
            if (y > 10000) break;
            cr->move_to(16, y);
            cr->show_text(line);
            y += line_height;
        }

        m_canvas.set_content_height(static_cast<int>(y + 24));
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
