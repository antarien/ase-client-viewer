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
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

class ViewerWindow : public Gtk::ApplicationWindow {
public:
    ViewerWindow() {
        set_title("ASE TECH & DESIGN Viewer");
        set_default_size(1400, 900);

        // HeaderBar
        auto header = Gtk::make_managed<Gtk::HeaderBar>();
        set_titlebar(*header);

        auto title_label = Gtk::make_managed<Gtk::Label>("ASE TECH & DESIGN Viewer");
        title_label->add_css_class("title");
        header->set_title_widget(*title_label);

        // Main layout: Paned (sidebar | content)
        m_paned.set_position(320);
        set_child(m_paned);

        // Sidebar placeholder
        auto sidebar_label = Gtk::make_managed<Gtk::Label>("Navigation");
        sidebar_label->set_margin(12);
        m_paned.set_start_child(*sidebar_label);

        // Content placeholder
        auto content_label = Gtk::make_managed<Gtk::Label>("Select a document to view");
        content_label->set_margin(12);
        m_paned.set_end_child(*content_label);
    }

private:
    Gtk::Paned m_paned{Gtk::Orientation::HORIZONTAL};
};

class ViewerApp : public Gtk::Application {
public:
    ViewerApp()
        : Gtk::Application("com.antarien.ase.viewer",
                           Gio::Application::Flags::HANDLES_OPEN) {}

protected:
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
                // Directory mode: will populate sidebar
                window->set_title("ASE Viewer — " + path);
            } else if (fs::is_regular_file(path)) {
                // Single file mode: will render immediately
                window->set_title("ASE Viewer — " + fs::path(path).filename().string());
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
