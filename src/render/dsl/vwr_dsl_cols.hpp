#pragma once

/**
 * @file        vwr_dsl_cols.hpp
 * @brief       render_columns - draws a :::columns directive block
 * @description Declares the single entry point the DSL dispatcher calls for a
 *              columns block. The body lives in vwr_dsl_cols.cpp, so a renderer
 *              that only needs the signature does not pull its implementation
 *              along.
 *
 * @module      ase-client-viewer
 * @layer       5 (Servers & Clients)
 * @category    render/ui/layout
 * @usecase     usecase:100008 - render the knowledge stock
 * @created     2026-04-13
 * @modified    2026-10-13
 * @version     00.00.35.00035 [core]
 */

#include "vwr_dsl_common.hpp"
namespace ase::viewer::render::dsl {
double render_columns(RenderContext& ctx, const ase::markdown::Node* node, int cw);
}
