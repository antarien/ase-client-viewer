/**
 * DSL leaf directive renderers.
 *
 * Single-line leaf directives that produce small inline-style block
 * elements: ::badge, ::divider, ::spacer, ::progress, ::kbd.
 *
 * @module      ase-client-viewer
 * @layer       5 (Clients)
 */

#include "vwr_dsl_leaf.hpp"

namespace ase::viewer::render::dsl {

double render_badge(RenderContext& ctx, const ase::markdown::Node* node, int cw) {
    (void)cw;
    const char* label_attr = get_attr(node, "label");
    std::string text = (label_attr != nullptr) ? std::string{label_attr}
                                               : build_inline_content(node);
    if (text.empty()) text = "badge";

    auto layout = create_body_layout(ctx.cr, cw);
    auto fd = layout->get_font_description();
    fd.set_size(9 * Pango::SCALE);
    fd.set_weight(Pango::Weight::BOLD);
    layout->set_font_description(fd);
    layout->set_markup(text);
    int tw = 0, th = 0;
    layout->get_pixel_size(tw, th);

    const double pad_x = 8.0, pad_y = 3.0;
    const double w = tw + 2 * pad_x;
    const double h = th + 2 * pad_y;

    ctx.cr->set_source_rgba(CYAN_R, CYAN_G, CYAN_B, 0.18);
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y, w, h, 3);
    ctx.cr->fill();
    ctx.cr->set_source_rgb(CYAN_R, CYAN_G, CYAN_B);
    draw_layout(ctx.cr, layout, ctx.margin_left + pad_x, ctx.y + pad_y);

    return h + 4;
}

double render_divider(RenderContext& ctx, int cw) {
    ctx.y += 8;
    ctx.cr->set_source_rgba(CYAN_R, CYAN_G, CYAN_B, 0.15);
    ctx.cr->set_line_width(1);
    ctx.cr->move_to(ctx.margin_left, ctx.y);
    ctx.cr->line_to(ctx.margin_left + cw, ctx.y);
    ctx.cr->stroke();
    return 16;
}

double render_spacer(RenderContext& ctx) {
    (void)ctx;
    return 32;
}

double render_progress(RenderContext& ctx, const ase::markdown::Node* node, int cw) {
    int value = get_attr_int(node, "value", 65);
    if (value < 0) value = 0;
    if (value > 100) value = 100;

    const double bar_h = 8.0;
    const double frac  = value / 100.0;

    ctx.cr->set_source_rgba(1.0, 1.0, 1.0, 0.05);
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y + 4, cw, bar_h, 4);
    ctx.cr->fill();

    ctx.cr->set_source_rgb(GREEN_R, GREEN_G, GREEN_B);
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y + 4, cw * frac, bar_h, 4);
    ctx.cr->fill();

    const char* label_attr = get_attr(node, "label");
    if (label_attr != nullptr) {
        std::string label_markup;
        ::ase::viewer::render::append_escaped(label_markup, label_attr);
        label_markup.append(" <span foreground=\"#80deff\">");
        label_markup.append(std::to_string(value));
        label_markup.append("%</span>");

        auto layout = create_body_layout(ctx.cr, cw);
        auto fd = layout->get_font_description();
        fd.set_size(9 * Pango::SCALE);
        layout->set_font_description(fd);
        layout->set_markup(label_markup);
        ctx.cr->set_source_rgb(MUTED_R, MUTED_G, MUTED_B);
        draw_layout(ctx.cr, layout, ctx.margin_left, ctx.y + 4 + bar_h + 4);
        return bar_h + 28;
    }
    return bar_h + 12;
}

double render_kbd(RenderContext& ctx, const ase::markdown::Node* node) {
    const char* keys_attr = get_attr(node, "keys");
    std::string text = (keys_attr != nullptr) ? std::string{keys_attr}
                                              : build_inline_content(node);
    if (text.empty()) text = "Ctrl+K";

    auto layout = create_body_layout(ctx.cr, 200);
    auto fd = layout->get_font_description();
    fd.set_family("Fira Code");
    fd.set_size(10 * Pango::SCALE);
    layout->set_font_description(fd);
    layout->set_markup(text);
    int tw = 0, th = 0;
    layout->get_pixel_size(tw, th);

    const double pad_x = 8.0, pad_y = 3.0;
    const double w = tw + 2 * pad_x;
    const double h = th + 2 * pad_y;

    ctx.cr->set_source_rgba(1.0, 1.0, 1.0, 0.06);
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y, w, h, 3);
    ctx.cr->fill();
    ctx.cr->set_source_rgba(1.0, 1.0, 1.0, 0.10);
    ctx.cr->set_line_width(0.5);
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y, w, h, 3);
    ctx.cr->stroke();

    ctx.cr->set_source_rgb(TEXT_R, TEXT_G, TEXT_B);
    draw_layout(ctx.cr, layout, ctx.margin_left + pad_x, ctx.y + pad_y);

    return h + 4;
}

}  // namespace ase::viewer::render::dsl
