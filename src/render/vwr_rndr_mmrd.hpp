#pragma once

/**
 * @file        vwr_rndr_mmrd.hpp
 * @brief       render_mermaid - draws a Mermaid diagram through Graphviz layout
 * @description Declares the entry point for a mermaid code block. The body
 *              parses the flowchart and sequence syntax into Graphviz DOT, runs
 *              the layout engine and draws boxes, edges and labels with Cairo.
 *              The caller passes the code bytes and gets back the height the
 *              diagram took.
 *
 * @module      ase-client-viewer
 * @layer       5 (Servers & Clients)
 * @category    render/draw/command
 * @actor       Text Renderer Framework (TXTRNDR)
 * @usecase     usecase:101419 - draw document text blocks
 * @created     2026-04-13
 * @modified    2026-10-13
 * @version     00.00.35.00035 [core]
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
