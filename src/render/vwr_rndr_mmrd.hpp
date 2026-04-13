#pragma once

/**
 * Mermaid diagram rendering via Graphviz layout engine.
 *
 * Parses Mermaid flowchart/sequence syntax into Graphviz DOT,
 * runs layout, draws boxes + edges + labels with Cairo.
 */

#include <cairomm/cairomm.h>
#include <cstdint>

namespace ase::viewer::render {

// Render a Mermaid diagram code block to Cairo.
// Returns total height consumed.
double render_mermaid(const Cairo::RefPtr<Cairo::Context>& cr,
                     const char* code, uint32_t len,
                     double x, double y, int max_width);

}  // namespace ase::viewer::render
