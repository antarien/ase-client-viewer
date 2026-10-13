#pragma once

/**
 * @file        vwr_rndr_amdl.hpp
 * @brief       render_ase_math_plot - draws an ASE-Math DSL plot with Cairo
 * @description Declares the entry point for an ase-math code block. The body
 *              evaluates the expressions through muparser and draws a 2D plot
 *              with Cairo: axes, curves, bands, markers and labels. The caller
 *              passes the code bytes and gets back the height the plot took, so
 *              the document walker can advance its cursor.
 *
 * @module      ase-client-viewer
 * @layer       5 (Servers & Clients)
 * @category    render/draw/command
 * @usecase     usecase:100008 - render the knowledge stock
 * @created     2026-04-13
 * @modified    2026-10-13
 * @version     00.00.35.00035 [core]
 */

#include <cairomm/cairomm.h>
#include <cstdint>

namespace ase::viewer::render {

// Render an ASE-Math plot from DSL code.
// Returns total height consumed.
double render_ase_math_plot(const Cairo::RefPtr<Cairo::Context>& cr,
                           const char* code, uint32_t len,
                           double x, double y, int max_width);

}  // namespace ase::viewer::render
