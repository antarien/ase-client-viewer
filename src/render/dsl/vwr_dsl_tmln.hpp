#pragma once

/**
 * @file        vwr_dsl_tmln.hpp
 * @brief       render_timeline - draws a :::timeline directive block
 * @description Declares the single entry point the DSL dispatcher calls for a
 *              timeline block. The body lives in vwr_dsl_tmln.cpp, so a
 *              renderer that only needs the signature does not pull its
 *              implementation along.
 *
 * @module      ase-client-viewer
 * @layer       5 (Servers & Clients)
 * @category    render/ui/widget
 * @usecase     usecase:100008 - render the knowledge stock
 * @created     2026-04-13
 * @modified    2026-10-13
 * @version     00.00.35.00035 [core]
 */

#include "vwr_dsl_common.hpp"
namespace ase::viewer::render::dsl {
double render_timeline(RenderContext& ctx, const ase::markdown::Node* node, int cw);
}
