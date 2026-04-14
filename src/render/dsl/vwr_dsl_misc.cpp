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
    // Fallback for unknown directives. No direct web counterpart — uses
    // the standard block scale (p-4 = 16, text-xs label, text-sm body)
    // so unknown blocks fit visually next to known ones.
    constexpr double PAD     = 16.0;
    constexpr int    LBL_PX  = 12;   // text-xs
    constexpr int    BODY_PX = 14;   // text-sm
    constexpr double GAP     = 4.0;

    std::string body_markup = build_inline_content(node);
    auto body_layout = create_body_layout(ctx.cr, cw - 2 * static_cast<int>(PAD));
    auto body_fd = body_layout->get_font_description();
    body_fd.set_size(BODY_PX * Pango::SCALE);
    body_layout->set_font_description(body_fd);
    body_layout->set_markup(body_markup);
    int bw = 0, bh = 0;
    body_layout->get_pixel_size(bw, bh);

    const double label_h = LBL_PX + GAP;
    const double total_h = (body_markup.empty() ? (label_h + 2 * PAD)
                                                : (label_h + bh + 2 * PAD));

    ctx.cr->set_source_rgba(PURPLE_R, PURPLE_G, PURPLE_B, 0.04);
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y, cw, total_h, 4);
    ctx.cr->fill();

    ctx.cr->set_source_rgba(PURPLE_R, PURPLE_G, PURPLE_B, 0.30);
    ctx.cr->rectangle(ctx.margin_left, ctx.y, 3, total_h);
    ctx.cr->fill();

    const char* name = (node->directive_name != nullptr) ? node->directive_name : "directive";
    auto label_layout = create_body_layout(ctx.cr, cw);
    auto fd = label_layout->get_font_description();
    fd.set_size(LBL_PX * Pango::SCALE);
    fd.set_weight(Pango::Weight::BOLD);
    label_layout->set_font_description(fd);
    label_layout->set_text(name);
    ctx.cr->set_source_rgb(PURPLE_R, PURPLE_G, PURPLE_B);
    draw_layout(ctx.cr, label_layout, ctx.margin_left + PAD, ctx.y + PAD);

    if (!body_markup.empty()) {
        ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_PANEL_MUTED),
                               rgb_g(::ase::colors::TEXT_PANEL_MUTED),
                               rgb_b(::ase::colors::TEXT_PANEL_MUTED));
        draw_layout(ctx.cr, body_layout, ctx.margin_left + PAD, ctx.y + PAD + label_h);
    }

    return total_h + 16.0;  // my-4 outer
}

}  // namespace ase::viewer::render::dsl
