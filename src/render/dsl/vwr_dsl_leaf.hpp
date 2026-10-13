#pragma once

/**
 * @file        vwr_dsl_leaf.hpp
 * @brief       render_badge and the other leaf directives of one line
 * @description Declares the entry points for the directives that carry their
 *              whole content on one line: badge, divider, spacer, progress, kbd
 *              and download. They share this header because they share the
 *              shape - no children to walk, one measured line to draw - and the
 *              bodies live in vwr_dsl_leaf.cpp.
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
double render_badge(RenderContext& ctx, const ase::markdown::Node* node, int cw);
double render_divider(RenderContext& ctx, int cw);
double render_spacer(RenderContext& ctx, const ase::markdown::Node* node);
double render_progress(RenderContext& ctx, const ase::markdown::Node* node, int cw);
double render_kbd(RenderContext& ctx, const ase::markdown::Node* node);
double render_download(RenderContext& ctx, const ase::markdown::Node* node, int cw);
}
