#pragma once

/**
 * @file        vwr_dsl_dispatch.hpp
 * @brief       render_directive - routes a directive node to its renderer
 * @description Declares the one entry point the document renderer calls for any
 *              block or leaf directive. The dispatcher reads the directive name
 *              and hands the node to the renderer that owns it, so the document
 *              walker knows no directive by name.
 *
 * @module      ase-client-viewer
 * @layer       5 (Servers & Clients)
 * @category    render/draw/command
 * @actor       Text Renderer Framework (TXTRNDR)
 * @usecase     usecase:101419 - draw document text blocks
 * @created     2026-04-12
 * @modified    2026-10-13
 * @version     00.00.35.00035 [core]
 */

#include "vwr_dsl_common.hpp"
namespace ase::viewer::render::dsl {
double render_directive(RenderContext& ctx, const ase::markdown::Node* node);
}
