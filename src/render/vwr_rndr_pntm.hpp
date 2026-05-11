#pragma once

/**
 * PlantUML diagram rendering via the `plantuml` CLI.
 *
 * The PlantUML DSL is forwarded as-is to the `plantuml -tsvg` binary
 * found on PATH; the resulting SVG is cached on disk (keyed by an
 * FNV-1a hash of the DSL bytes) and drawn through ase::imgcache, which
 * already handles SVG decoding via Gdk::Pixbuf.
 *
 * No library link — the renderer probes for the binary at runtime
 * and falls back to a textual placeholder when `plantuml` is absent.
 */

#include <cairomm/cairomm.h>
#include <cstdint>

namespace ase::viewer::render {

// Render a PlantUML diagram code block to Cairo.
// Returns total height consumed.
double render_plantuml(const Cairo::RefPtr<Cairo::Context>& cr,
                       const char* code, uint32_t len,
                       double x, double y, int max_width);

}  // namespace ase::viewer::render
