#pragma once
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
