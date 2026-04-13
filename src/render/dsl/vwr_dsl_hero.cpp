/**
 * DSL hero directive renderer.
 *
 * :::hero{title="…"}
 *   subtitle / description content
 * :::
 *
 * Renders a full-width banner with a tinted background, left accent bar
 * and a large H1-style heading. Body content (any inline markdown that is
 * not a child directive) is rendered as a body Pango layout below the
 * title — bold, italic, code, NerdFont icons all preserved through the
 * shared inline markup builder.
 *
 * @module      ase-client-viewer
 * @layer       5 (Clients)
 */

#include "vwr_dsl_hero.hpp"

namespace ase::viewer::render::dsl {

double render_hero(RenderContext& ctx, const ase::markdown::Node* node, int cw) {
    const char* title_attr = get_attr(node, "title");

    std::string title_markup;
    if (title_attr != nullptr) {
        ::ase::viewer::render::append_escaped(title_markup, title_attr);
    } else {
        title_markup = build_inline_content(node);
    }

    auto title_layout = create_heading_layout(ctx.cr, cw - 48, 1);
    title_layout->set_markup(title_markup);
    int tw = 0, th = 0;
    title_layout->get_pixel_size(tw, th);

    std::string body_markup;
    if (title_attr != nullptr) {
        body_markup = build_inline_content(node);
    }
    auto body_layout = create_body_layout(ctx.cr, cw - 48);
    body_layout->set_markup(body_markup);
    int bw = 0, bh = 0;
    body_layout->get_pixel_size(bw, bh);

    const double v_pad = 24.0;
    const double gap   = body_markup.empty() ? 0.0 : 10.0;
    const double total_h = v_pad + th + gap + bh + v_pad;

    ctx.cr->set_source_rgba(CYAN_R, CYAN_G, CYAN_B, 0.06);
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y, cw, total_h, 4);
    ctx.cr->fill();

    ctx.cr->set_source_rgb(CYAN_R, CYAN_G, CYAN_B);
    ctx.cr->rectangle(ctx.margin_left, ctx.y, 3, total_h);
    ctx.cr->fill();

    ctx.cr->set_source_rgb(CYAN_R, CYAN_G, CYAN_B);
    draw_layout(ctx.cr, title_layout, ctx.margin_left + 24, ctx.y + v_pad);

    if (!body_markup.empty()) {
        ctx.cr->set_source_rgb(TEXT_R, TEXT_G, TEXT_B);
        draw_layout(ctx.cr, body_layout, ctx.margin_left + 24, ctx.y + v_pad + th + gap);
    }

    return total_h + PARA_SPACING;
}

}  // namespace ase::viewer::render::dsl
