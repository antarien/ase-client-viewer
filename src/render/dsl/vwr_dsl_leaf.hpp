#pragma once
#include "vwr_dsl_common.hpp"
namespace ase::viewer::render::dsl {
double render_badge(RenderContext& ctx, const ase::markdown::Node* node, int cw);
double render_divider(RenderContext& ctx, int cw);
double render_spacer(RenderContext& ctx, const ase::markdown::Node* node);
double render_progress(RenderContext& ctx, const ase::markdown::Node* node, int cw);
double render_kbd(RenderContext& ctx, const ase::markdown::Node* node);
double render_download(RenderContext& ctx, const ase::markdown::Node* node, int cw);
}
