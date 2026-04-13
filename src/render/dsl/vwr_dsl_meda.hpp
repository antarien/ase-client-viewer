#pragma once
#include "vwr_dsl_common.hpp"
namespace ase::viewer::render::dsl {
double render_callout(RenderContext& ctx, const ase::markdown::Node* node, int cw);
double render_quote(RenderContext& ctx, const ase::markdown::Node* node, int cw);
double render_aside(RenderContext& ctx, const ase::markdown::Node* node, int cw);
double render_terminal(RenderContext& ctx, const ase::markdown::Node* node, int cw);
double render_matrix(RenderContext& ctx, const ase::markdown::Node* node, int cw);
}
