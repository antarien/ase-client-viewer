#pragma once

/**
 * ASE-Math DSL plot renderer.
 *
 * Evaluates mathematical expressions via muparser and renders
 * 2D plots with Cairo: axes, curves, bands, markers, labels.
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
