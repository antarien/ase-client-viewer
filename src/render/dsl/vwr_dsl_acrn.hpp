#pragma once

/**
 * @file        vwr_dsl_acrn.hpp
 * @brief       render_accordion - draws a :::accordion directive block
 * @description Declares the single entry point the DSL dispatcher calls for an
 *              accordion block. The body lives in vwr_dsl_acrn.cpp, so a
 *              renderer that only needs the accordion's signature does not pull
 *              its implementation along.
 *
 * @module      ase-client-viewer
 * @layer       5 (Servers & Clients)
 * @category    render/ui/widget
 * @actor       Text Renderer Framework (TXTRNDR)
 * @usecase     usecase:101419 - draw document text blocks
 * @created     2026-04-13
 * @modified    2026-10-13
 * @version     00.00.35.00035 [core]
 */

#include "vwr_dsl_common.hpp"
namespace ase::viewer::render::dsl {
double render_accordion(RenderContext& ctx, const ase::markdown::Node* node, int cw);
}
