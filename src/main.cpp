/**
 * @file        main.cpp
 * @brief       ASE TECH & DESIGN Viewer - entry point and lifecycle hooks
 * @description Pure orchestrator: creates the ase::gtk::Application, installs
 *              the dark Adwaita color scheme and CSS once at startup, and
 *              hands every activate/open event to a freshly-created
 *              ViewerWindow. No UI logic, no feature code, no file system
 *              work lives in this file - everything is delegated to the
 *              feature modules under include/viewer and src/.
 *
 * Start modes:
 *   ase-viewer /path/to/docs/   : opens the given directory as root
 *   ase-viewer document.md      : opens a single markdown file
 *   ase-viewer                  : opens <ase>/docs/ase-docs/tech
 *
 * @module      ase-client-viewer
 * @layer       5
 */

#include <viewer/fs_utils.hpp>
#include <viewer/theme.hpp>
#include <viewer/window.hpp>

#include <ase/adp/adw/adw.hpp>
#include <ase/adp/gtk/application.hpp>
#include <ase/adp/gtk/style.hpp>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

std::unique_ptr<ase::viewer::ViewerWindow> make_window(ase::gtk::Application& app) {
    auto window = ase::gtk::ApplicationWindow::create(app);
    auto viewer_window = std::make_unique<ase::viewer::ViewerWindow>(std::move(window));
    viewer_window->build_ui();
    return viewer_window;
}

std::string default_docs_root() {
    // <ase>/docs/ase-docs/tech relative to the running binary. The
    // binary lives under <ase>/build/bin/ase-viewer, so five parent_dir
    // hops land us at <ase>.
    const std::string exe_path = ase::viewer::fs::read_symlink_target("/proc/self/exe");
    std::string base = exe_path;
    for (int i = 0; i < 5; ++i) base = ase::viewer::fs::parent_dir(base);
    return ase::viewer::fs::path_join(
        ase::viewer::fs::path_join(
            ase::viewer::fs::path_join(base, "docs"),
            "ase-docs"),
        "tech");
}

}  // namespace

int main(int argc, char* argv[]) {
    auto app = ase::gtk::Application::create(
        "com.antarien.ase.viewer",
        ase::gtk::Application::Flags::HandlesOpen);

    // Shared slot holding the currently-presented window so the app keeps it
    // alive for its whole lifetime.
    auto current_window = std::make_shared<std::unique_ptr<ase::viewer::ViewerWindow>>();

    app.on_startup([]() {
        ase::adw::style_manager::init();
        ase::adw::style_manager::set_color_scheme(ase::adw::ColorScheme::ForceDark);

        auto css = ase::gtk::CssProvider::create();
        css.load_from_data(ase::viewer::theme::generate_css());
        css.install_for_default_display();
    });

    app.on_activate([&app, current_window]() {
        auto win = make_window(app);
        const std::string docs = default_docs_root();
        if (ase::viewer::fs::is_directory(docs)) {
            win->load_directory(docs);
        }
        win->present();
        *current_window = std::move(win);
    });

    app.on_open([&app, current_window](const std::vector<std::string>& paths) {
        auto win = make_window(app);
        if (!paths.empty()) {
            const std::string& first = paths.front();
            if (ase::viewer::fs::is_directory(first)) {
                win->load_directory(first);
            } else if (ase::viewer::fs::is_regular_file(first)) {
                win->load_file(first);
            }
        }
        win->present();
        *current_window = std::move(win);
    });

    return app.run(argc, argv);
}
