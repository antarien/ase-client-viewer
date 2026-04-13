#pragma once

/**
 * ASE Viewer — LaTeX math renderer (thin wrapper over ase::microtex)
 *
 * Historic header that exposed a hand-rolled KaTeX subset is replaced
 * with a thin shim around the ase::microtex adapter. All LaTeX parsing,
 * box-tree layout, and drawing happens inside the adapter module;
 * this file keeps the historic public signature so existing viewer
 * call sites do not need to change.
 *
 * @module      ase-client-viewer
 * @layer       5 (Clients)
 */

#include <cairomm/cairomm.h>
#include <cstdint>

namespace ase::viewer::render {

struct MathResult {
    double width    = 0.0;
    double height   = 0.0;
    double depth    = 0.0;
    double baseline = 0.0;
};

MathResult render_math(
    const Cairo::RefPtr<Cairo::Context>& cr,
    const char* latex,
    uint32_t latex_len,
    double x,
    double y,
    double font_size,
    bool is_display
);

// Measure-only — no draw. Used for inline math layout where the caller
// reserves a Pango shape box before drawing the actual glyphs.
MathResult measure_math(
    const char* latex,
    uint32_t latex_len,
    double font_size,
    bool is_display
);

}  // namespace ase::viewer::render
