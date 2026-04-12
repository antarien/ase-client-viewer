#pragma once

/**
 * DSL directive dispatcher — routes directive nodes to their
 * specialized renderers based on directive name.
 */

#include "../vwr_rndr_doc.hpp"
#include <ase/markdown/markdown.hpp>

namespace ase::viewer::render::dsl {

// Render a directive node (block, leaf, or inline).
// Returns height consumed.
double render_directive(RenderContext& ctx, const ase::markdown::Node* node);

}  // namespace ase::viewer::render::dsl
