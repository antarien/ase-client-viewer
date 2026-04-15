#pragma once
#include "vwr_dsl_common.hpp"
namespace ase::viewer::render::dsl {
double render_generic(RenderContext& ctx, const ase::markdown::Node* node, int cw);
double render_toc(RenderContext& ctx, const ase::markdown::Node* node, int cw);
double render_changelog(RenderContext& ctx, const ase::markdown::Node* node, int cw);
double render_team(RenderContext& ctx, const ase::markdown::Node* node, int cw);
double render_author(RenderContext& ctx, const ase::markdown::Node* node, int cw);
}
