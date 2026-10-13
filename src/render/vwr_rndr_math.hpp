#pragma once

/**
 * @file        vwr_rndr_math.hpp
 * @brief       render_math and measure_math - LaTeX through ase::adp::microtex
 * @description A thin shim over the ase::adp::microtex adapter. All LaTeX
 *              parsing, box-tree layout and drawing happen inside the adapter
 *              module; this header carries the signature and the result struct
 *              the viewer's call sites use.
 *
 *              MEASURE AND DRAW ARE TWO CALLS because inline math needs its
 *              box before anything is painted: the caller reserves a Pango
 *              shape box from measure_math and draws the glyphs afterwards.
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
