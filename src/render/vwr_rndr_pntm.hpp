#pragma once

/**
 * @file        vwr_rndr_pntm.hpp
 * @brief       render_plantuml - draws a PlantUML diagram through the CLI
 * @description Declares the entry point for a plantuml code block. The DSL is
 *              forwarded as-is to the `plantuml -tsvg` binary found on PATH;
 *              the resulting SVG is cached on disk, keyed by an FNV-1a hash of
 *              the DSL bytes, and drawn through ase::imgcache, which already
 *              decodes SVG via Gdk::Pixbuf.
 *
 *              NO LIBRARY IS LINKED FOR IT: the renderer probes for the binary
 *              at runtime and falls back to a textual placeholder when
 *              `plantuml` is absent, so a missing tool costs a placeholder and
 *              not the document.
 *
 * @module      ase-client-viewer
 * @layer       5 (Servers & Clients)
 * @category    render/draw/command
 * @actor       Text Renderer Framework (TXTRNDR)
 * @usecase     usecase:101419 - draw document text blocks
 * @created     2026-05-11
 * @modified    2026-10-13
 * @version     00.00.35.00035 [core]
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
