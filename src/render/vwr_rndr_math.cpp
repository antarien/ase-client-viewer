/**
 * ASE Viewer — LaTeX math renderer (thin wrapper over ase::adp::microtex)
 *
 * The previous hand-rolled KaTeX subset (600+ lines of tokenizer /
 * box-tree / Cairo draw code) is replaced by a one-call delegation to
 * the ase-adp-microtex which drives the full NanoMichael/MicroTeX
 * engine. Matrix / align / stretchy-delimiter coverage now comes for
 * free. The public signature and struct layout are preserved so
 * existing callers in vwr_rndr_doc.cpp do not need to change.
 *
 * @module      ase-client-viewer
 * @layer       5 (Clients)
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
