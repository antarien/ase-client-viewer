#pragma once

/**
 * Document renderer — walks the ase::markdown AST and dispatches
 * Cairo draw calls for each node type. Computes layout (y-positions)
 * and renders visible nodes within the viewport.
 */

#include <ase/markdown/markdown.hpp>
#include <cairomm/cairomm.h>
#include <pangomm.h>
#include <cstdint>

namespace ase::viewer::render {

struct RenderContext {
    Cairo::RefPtr<Cairo::Context> cr;
    int width          = 0;
    int margin_left    = 16;
    int margin_right   = 16;
    int margin_top     = 16;
    double y           = 0;    // Current y position (accumulates)
    double scroll_y    = 0;    // Scroll offset
    double viewport_h  = 0;    // Visible height
};

// Render the full document AST into the Cairo context.
// Returns total content height (for scrollbar).
double render_document(RenderContext& ctx, const ase::markdown::Document& doc);

// Render a single AST node (recursive).
void render_node(RenderContext& ctx, const ase::markdown::Node* node);

}  // namespace ase::viewer::render
