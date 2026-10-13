#pragma once

/**
 * @file        vwr_dsl_meda.hpp
 * @brief       render_callout and the other framed and media directives
 * @description Declares the entry points for the directives that frame their
 *              content or carry a medium: callout, quote, aside, terminal,
 *              matrix, figure, gallery, video, audio and compare. The bodies
 *              live in vwr_dsl_meda.cpp.
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
double render_callout(RenderContext& ctx, const ase::markdown::Node* node, int cw);
double render_quote(RenderContext& ctx, const ase::markdown::Node* node, int cw);
double render_aside(RenderContext& ctx, const ase::markdown::Node* node, int cw);
double render_terminal(RenderContext& ctx, const ase::markdown::Node* node, int cw);
double render_matrix(RenderContext& ctx, const ase::markdown::Node* node, int cw);
double render_figure(RenderContext& ctx, const ase::markdown::Node* node, int cw);
double render_gallery(RenderContext& ctx, const ase::markdown::Node* node, int cw);
double render_video(RenderContext& ctx, const ase::markdown::Node* node, int cw);
double render_audio(RenderContext& ctx, const ase::markdown::Node* node, int cw);
double render_compare(RenderContext& ctx, const ase::markdown::Node* node, int cw);
}
