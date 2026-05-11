/*
 * ==============================================================================
 * ASE VIEWER — PlantUML renderer
 * ==============================================================================
 *
 * @file        vwr_rndr_pntm.cpp
 * @brief       PlantUML code block to SVG to Cairo renderer
 * @description Forwards a PlantUML DSL block to the `plantuml -tsvg` binary
 *              (probed on PATH at runtime), caches the resulting SVG on
 *              disk under $XDG_CACHE_HOME/ase-viewer/plantuml/<fnv1a>.svg,
 *              and hands the cached path to ase::imgcache::render() —
 *              which already decodes SVG through Gdk::Pixbuf for the
 *              figure/image renderers. No library link required: a missing
 *              `plantuml` binary degrades gracefully to a textual
 *              placeholder, identical in behaviour to the HAVE_GRAPHVIZ
 *              fallback in vwr_rndr_mmrd.cpp.
 *
 * @module      ase-client-viewer
 * @layer       5 (Clients)
 * @category    process/computation/algorithm
 *
 * @created     2026-05-11
 * @modified    2026-05-11
 * @version     00.00.01.00001 [seed]
 *
 * ==============================================================================
 * CLIENT INFRASTRUCTURE COMPLIANCE
 * ==============================================================================
 * [ ] No raw new/delete (smart pointers or stack only)
 * [ ] No std::vector / std::map / std::set in source-level data structures
 * [ ] No switch/case dispatch — use if/else ladders
 * [ ] No std::stringstream / std::strcmp / std::strlen
 * [ ] PlantUML invocation goes through fork()+execvp() (no shell parsing)
 * [ ] Stdout / stderr of the child silenced to /dev/null
 * [ ] SVG cache keyed by FNV-1a of the DSL bytes (deterministic, collision-tolerant)
 * [ ] Cache directory rooted at $XDG_CACHE_HOME (g_get_user_cache_dir())
 * [ ] Placeholder fallback when `plantuml` is not on PATH
 * ==============================================================================
 */

#include "vwr_rndr_pntm.hpp"

#include <ase/imgcache/image_cache.hpp>

#include <pangomm.h>
#include <glib.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace ase::viewer::render {

namespace {

constexpr double MUTED_R = 0.353, MUTED_G = 0.353, MUTED_B = 0.353;
constexpr double CYAN_R  = 0.353, CYAN_G  = 0.612, CYAN_B  = 0.722;

// FNV-1a 64-bit. Stable across runs — the SVG cache filename only needs
// determinism plus collision tolerance at the cache scale we operate at
// (a few diagrams per document, thousands per session).
uint64_t fnv1a_64(const char* data, uint32_t len) {
    uint64_t h = 0xcbf29ce484222325ULL;
    for (uint32_t i = 0; i < len; ++i) {
        h ^= static_cast<uint8_t>(data[i]);
        h *= 0x100000001b3ULL;
    }
    return h;
}

// Build the SVG cache path for a given DSL hash. Creates the directory
// on first use; returns the .svg path (the .puml staging file lives
// next to it).
std::string cache_paths(uint64_t hash, std::string& puml_out, std::string& svg_out) {
    const char* cache_root = g_get_user_cache_dir();
    std::string dir = std::string{cache_root} + "/ase-viewer/plantuml";
    g_mkdir_with_parents(dir.c_str(), 0700);

    char namebuf[32];
    g_snprintf(namebuf, sizeof namebuf, "%016llx",
               static_cast<unsigned long long>(hash));

    puml_out = dir + "/" + namebuf + ".puml";
    svg_out  = dir + "/" + namebuf + ".svg";
    return svg_out;
}

// Write the DSL bytes verbatim to `path`. Returns true on success.
bool write_puml(const std::string& path, const char* code, uint32_t len) {
    FILE* f = std::fopen(path.c_str(), "wb");
    if (f == nullptr) return false;
    const size_t written = std::fwrite(code, 1, len, f);
    std::fclose(f);
    return written == len;
}

// Spawn `plantuml -tsvg -quiet <puml_path>` and wait for it. Child's
// stdout / stderr are silenced so the parent terminal stays clean. The
// PlantUML binary writes the .svg next to the input .puml.
bool run_plantuml(const std::string& puml_path) {
    const pid_t pid = fork();
    if (pid < 0) return false;
    if (pid == 0) {
        const int devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0) {
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
        execlp("plantuml", "plantuml", "-tsvg", "-quiet",
               puml_path.c_str(), nullptr);
        _exit(127);
    }
    int status = 0;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) return false;
    }
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

bool plantuml_on_path() {
    gchar* p = g_find_program_in_path("plantuml");
    if (p == nullptr) return false;
    g_free(p);
    return true;
}

// Draw a small placeholder when `plantuml` is missing or the conversion
// failed. Mirrors the muted-cyan rail used by the Mermaid fallback so
// the two diagnostics read identically.
double draw_placeholder(const Cairo::RefPtr<Cairo::Context>& cr,
                        const char* code, uint32_t len,
                        double x, double y, int max_width,
                        const std::string& title) {
    cr->set_source_rgba(CYAN_R, CYAN_G, CYAN_B, 0.06);
    cr->rectangle(x, y, max_width, 60);
    cr->fill();
    cr->set_source_rgb(CYAN_R, CYAN_G, CYAN_B);
    cr->rectangle(x, y, 3, 60);
    cr->fill();

    auto layout = Pango::Layout::create(cr);
    Pango::FontDescription fd;
    fd.set_family("Fira Code");
    fd.set_absolute_size(10 * Pango::SCALE);
    layout->set_font_description(fd);
    layout->set_text(title);
    cr->set_source_rgb(MUTED_R, MUTED_G, MUTED_B);
    cr->move_to(x + 12, y + 6);
    layout->show_in_cairo_context(cr);

    const char* p = code;
    const char* end = code + len;
    double ly = y + 24;
    int line_count = 0;
    while (line_count < 3 && p < end) {
        const char* line_start = p;
        while (p < end && *p != '\n') ++p;
        const uint32_t line_len = static_cast<uint32_t>(p - line_start);
        if (p < end) ++p;

        auto code_layout = Pango::Layout::create(cr);
        Pango::FontDescription cfd;
        cfd.set_family("Fira Code");
        cfd.set_absolute_size(9 * Pango::SCALE);
        code_layout->set_font_description(cfd);
        code_layout->set_text(std::string(line_start, line_len));
        cr->set_source_rgb(MUTED_R, MUTED_G, MUTED_B);
        cr->move_to(x + 12, ly);
        code_layout->show_in_cairo_context(cr);
        ly += 14;
        ++line_count;
    }
    return 60 + line_count * 14;
}

}  // namespace

double render_plantuml(const Cairo::RefPtr<Cairo::Context>& cr,
                       const char* code, uint32_t len,
                       double x, double y, int max_width) {
    if (code == nullptr || len == 0) return 0.0;

    if (!plantuml_on_path()) {
        return draw_placeholder(cr, code, len, x, y, max_width,
                                "plantuml diagram (plantuml binary not on PATH)");
    }

    const uint64_t hash = fnv1a_64(code, len);
    std::string puml_path;
    std::string svg_path;
    cache_paths(hash, puml_path, svg_path);

    if (!g_file_test(svg_path.c_str(), G_FILE_TEST_EXISTS)) {
        if (!write_puml(puml_path, code, len)) {
            return draw_placeholder(cr, code, len, x, y, max_width,
                                    "plantuml diagram (cache write failed)");
        }
        if (!run_plantuml(puml_path)) {
            return draw_placeholder(cr, code, len, x, y, max_width,
                                    "plantuml diagram (plantuml exec failed)");
        }
    }

    if (!g_file_test(svg_path.c_str(), G_FILE_TEST_EXISTS)) {
        return draw_placeholder(cr, code, len, x, y, max_width,
                                "plantuml diagram (no svg produced)");
    }

    return ::ase::imgcache::render(cr, svg_path, x, y, max_width);
}

}  // namespace ase::viewer::render
