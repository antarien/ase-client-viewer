#pragma once

/**
 * @file        vwr_dsl_misc.hpp
 * @brief       render_generic and the directives built from a list
 * @description Declares the fallback renderer for a directive nobody owns, plus
 *              the ones whose content is a list the renderer walks: toc,
 *              changelog, team and author. The bodies live in
 *              vwr_dsl_misc.cpp.
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
double render_generic(RenderContext& ctx, const ase::markdown::Node* node, int cw);
double render_toc(RenderContext& ctx, const ase::markdown::Node* node, int cw);
double render_changelog(RenderContext& ctx, const ase::markdown::Node* node, int cw);
double render_team(RenderContext& ctx, const ase::markdown::Node* node, int cw);
double render_author(RenderContext& ctx, const ase::markdown::Node* node, int cw);
}
