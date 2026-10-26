/**
 * @file        vwr_rndr_math.cpp
 * @brief       LaTeX math delegated in one call to ase-adp-microtex
 * @description Each entry point hands its arguments to the adapter, which
 *              drives the full NanoMichael/MicroTeX engine, and copies the
 *              result into this module's own struct. Matrix, align and
 *              stretchy-delimiter coverage therefore come from the engine
 *              rather than from a subset maintained here.
 *
 *              The struct is copied field by field instead of returned
 *              straight through: the viewer's result type belongs to the
 *              viewer, so a change in the adapter's type cannot reach the
 *              callers in vwr_rndr_doc.cpp unnoticed.
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

#include "vwr_rndr_math.hpp"

#include <ase/adp/microtex/render.hpp>

namespace ase::viewer::render {

MathResult render_math(
    const Cairo::RefPtr<Cairo::Context>& cr,
    const char* latex,
    uint32_t latex_len,
    double x,
    double y,
    double font_size,
    bool is_display
) {
    const ase::adp::microtex::MathResult r =
        ase::adp::microtex::render_math(cr, latex, latex_len, x, y, font_size, is_display);

    MathResult out;
    out.width    = r.width;
    out.height   = r.height;
    out.depth    = r.depth;
    out.baseline = r.baseline;
    return out;
}

MathResult measure_math(
    const char* latex,
    uint32_t latex_len,
    double font_size,
    bool is_display
) {
    const ase::adp::microtex::MathResult r =
        ase::adp::microtex::measure_math(latex, latex_len, font_size, is_display);

    MathResult out;
    out.width    = r.width;
    out.height   = r.height;
    out.depth    = r.depth;
    out.baseline = r.baseline;
    return out;
}

}  // namespace ase::viewer::render
