/**
 * DSL generic / fallback directive renderer.
 *
 * Used by the dispatcher when no specialist renderer matches the
 * directive name. Renders the inline children inside a tinted purple
 * panel with the directive name as a small label so unsupported
 * directives stay visible (instead of silently becoming empty boxes).
 *
 * @module      ase-client-viewer
 * @layer       5 (Clients)
 */

#include "vwr_dsl_misc.hpp"

namespace ase::viewer::render::dsl {

double render_generic(RenderContext& ctx, const ase::markdown::Node* node, int cw) {
    std::string body_markup = build_inline_content(node);
    auto body_layout = create_body_layout(ctx.cr, cw - 24);
    body_layout->set_markup(body_markup);
    int bw = 0, bh = 0;
    body_layout->get_pixel_size(bw, bh);

    const double label_h = 18.0;
    const double total_h = (body_markup.empty() ? 32.0 : (bh + 2 * BLOCK_PAD + label_h));

    ctx.cr->set_source_rgba(PURPLE_R, PURPLE_G, PURPLE_B, 0.04);
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y, cw, total_h, 4);
    ctx.cr->fill();

    ctx.cr->set_source_rgba(PURPLE_R, PURPLE_G, PURPLE_B, 0.30);
    ctx.cr->rectangle(ctx.margin_left, ctx.y, 3, total_h);
    ctx.cr->fill();

    const char* name = (node->directive_name != nullptr) ? node->directive_name : "directive";
    auto label_layout = create_body_layout(ctx.cr, cw);
    auto fd = label_layout->get_font_description();
    fd.set_size(9 * Pango::SCALE);
    fd.set_weight(Pango::Weight::BOLD);
    label_layout->set_font_description(fd);
    label_layout->set_text(name);
    ctx.cr->set_source_rgb(PURPLE_R, PURPLE_G, PURPLE_B);
    draw_layout(ctx.cr, label_layout, ctx.margin_left + 12, ctx.y + 4);

    if (!body_markup.empty()) {
        ctx.cr->set_source_rgb(TEXT_R, TEXT_G, TEXT_B);
        draw_layout(ctx.cr, body_layout, ctx.margin_left + 12, ctx.y + label_h + 6);
    }

    return total_h + PARA_SPACING;
}

}  // namespace ase::viewer::render::dsl
