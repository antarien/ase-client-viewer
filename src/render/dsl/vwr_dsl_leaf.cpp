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

// Web 1:1 leaf primitives.
//   badge    px-2 py-0.5 text-xs (8/2/12)
//   divider  my-6 border-top BORDER_A30 (24)
//   spacer   my-8 (32)
//   progress h-2 (8) bar, label text-xs (12)
//   kbd      px-2 py-0.5 text-xs (8/2/12)

double render_badge(RenderContext& ctx, const ase::markdown::Node* node, int cw) {
    (void)cw;
    constexpr double PAD_X   = 8.0;   // px-2
    constexpr double PAD_Y   = 2.0;   // py-0.5
    constexpr int    TXT_PX  = 12;    // text-xs

    const char* label_attr = get_attr(node, "label");
    std::string text = (label_attr != nullptr) ? std::string{label_attr}
                                               : build_inline_content(node);
    if (text.empty()) text = "badge";

    auto layout = create_body_layout(ctx.cr, cw);
    auto fd = layout->get_font_description();
    fd.set_size(TXT_PX * Pango::SCALE);
    fd.set_weight(Pango::Weight::BOLD);
    layout->set_font_description(fd);
    layout->set_markup(text);
    int tw = 0, th = 0;
    layout->get_pixel_size(tw, th);

    const double w = tw + 2 * PAD_X;
    const double h = th + 2 * PAD_Y;

    ctx.cr->set_source_rgba(CYAN_R, CYAN_G, CYAN_B, 0.18);
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y, w, h, 3);
    ctx.cr->fill();
    ctx.cr->set_source_rgb(CYAN_R, CYAN_G, CYAN_B);
    draw_layout(ctx.cr, layout, ctx.margin_left + PAD_X, ctx.y + PAD_Y);

    return h + 4;
}

double render_divider(RenderContext& ctx, int cw) {
    // Web <hr> my-6 = 24 vertical, border-top BORDER_A30.
    constexpr double MY = 24.0;
    ctx.y += MY / 2.0;
    ctx.cr->set_source_rgba(rgb_r(::ase::colors::BORDER_A30),
                            rgb_g(::ase::colors::BORDER_A30),
                            rgb_b(::ase::colors::BORDER_A30),
                            rgb_a(::ase::colors::BORDER_A30));
    ctx.cr->set_line_width(1);
    ctx.cr->move_to(ctx.margin_left, ctx.y);
    ctx.cr->line_to(ctx.margin_left + cw, ctx.y);
    ctx.cr->stroke();
    return MY;
}

double render_spacer(RenderContext& ctx) {
    (void)ctx;
    return 32;  // my-8
}

double render_progress(RenderContext& ctx, const ase::markdown::Node* node, int cw) {
    constexpr double BAR_H  = 8.0;   // h-2
    constexpr int    TXT_PX = 12;    // text-xs
    constexpr double GAP    = 4.0;

    int value = get_attr_int(node, "value", 65);
    if (value < 0) value = 0;
    if (value > 100) value = 100;
    const double frac = value / 100.0;

    ctx.cr->set_source_rgb(rgb_r(::ase::colors::SURFACE_1_ALT),
                           rgb_g(::ase::colors::SURFACE_1_ALT),
                           rgb_b(::ase::colors::SURFACE_1_ALT));
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y + GAP, cw, BAR_H, 4);
    ctx.cr->fill();

    ctx.cr->set_source_rgb(GREEN_R, GREEN_G, GREEN_B);
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y + GAP, cw * frac, BAR_H, 4);
    ctx.cr->fill();

    const char* label_attr = get_attr(node, "label");
    if (label_attr != nullptr) {
        std::string label_markup;
        ::ase::viewer::render::append_escaped(label_markup, label_attr);
        label_markup.append(" <span foreground=\"");
        label_markup.append(color_hex(::ase::colors::DOCS_H1));
        label_markup.append("\">");
        label_markup.append(std::to_string(value));
        label_markup.append("%</span>");

        auto layout = create_body_layout(ctx.cr, cw);
        auto fd = layout->get_font_description();
        fd.set_size(TXT_PX * Pango::SCALE);
        layout->set_font_description(fd);
        layout->set_markup(label_markup);
        ctx.cr->set_source_rgb(MUTED_R, MUTED_G, MUTED_B);
        draw_layout(ctx.cr, layout, ctx.margin_left, ctx.y + GAP + BAR_H + GAP);
        return BAR_H + 32;
    }
    return BAR_H + 12;
}

double render_kbd(RenderContext& ctx, const ase::markdown::Node* node) {
    constexpr double PAD_X  = 8.0;   // px-2
    constexpr double PAD_Y  = 2.0;   // py-0.5
    constexpr int    TXT_PX = 12;    // text-xs

    const char* keys_attr = get_attr(node, "keys");
    std::string text = (keys_attr != nullptr) ? std::string{keys_attr}
                                              : build_inline_content(node);
    if (text.empty()) text = "Ctrl+K";

    auto layout = create_body_layout(ctx.cr, 200);
    auto fd = layout->get_font_description();
    fd.set_family("Fira Code");
    fd.set_size(TXT_PX * Pango::SCALE);
    layout->set_font_description(fd);
    layout->set_markup(text);
    int tw = 0, th = 0;
    layout->get_pixel_size(tw, th);

    const double w = tw + 2 * PAD_X;
    const double h = th + 2 * PAD_Y;

    ctx.cr->set_source_rgb(rgb_r(::ase::colors::SURFACE_2),
                           rgb_g(::ase::colors::SURFACE_2),
                           rgb_b(::ase::colors::SURFACE_2));
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y, w, h, 3);
    ctx.cr->fill();
    ctx.cr->set_source_rgba(rgb_r(::ase::colors::BORDER_A20),
                            rgb_g(::ase::colors::BORDER_A20),
                            rgb_b(::ase::colors::BORDER_A20),
                            rgb_a(::ase::colors::BORDER_A20));
    ctx.cr->set_line_width(1);
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y, w, h, 3);
    ctx.cr->stroke();

    ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_PANEL_WHITE),
                           rgb_g(::ase::colors::TEXT_PANEL_WHITE),
                           rgb_b(::ase::colors::TEXT_PANEL_WHITE));
    draw_layout(ctx.cr, layout, ctx.margin_left + PAD_X, ctx.y + PAD_Y);

    return h + 4;
}

}  // namespace ase::viewer::render::dsl
