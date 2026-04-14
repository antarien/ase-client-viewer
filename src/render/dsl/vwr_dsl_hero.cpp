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
    // Web 1:1 (cmsHeroBlock.tsx):
    //   padding: p-8 = 32 px all sides
    //   border-radius: 6
    //   min-height: 280
    //   background: rgba(4,4,4,0.75) overlay on bg image — native uses
    //               SURFACE_0_PANEL (#040404) directly since no image layer
    //   text color: CMS_HERO_TEXT (#ffffff)
    //   heading + body each have mb-2 = 8 px bottom margin
    constexpr double PAD       = 32.0;
    constexpr double GAP       = 8.0;
    constexpr double MIN_H     = 280.0;
    constexpr double RADIUS    = 6.0;
    constexpr double TEXT_PAD  = PAD;  // text x offset = padding

    const char* title_attr = get_attr(node, "title");

    std::string title_markup;
    if (title_attr != nullptr) {
        ::ase::viewer::render::append_escaped(title_markup, title_attr);
    } else {
        title_markup = build_inline_content(node);
    }

    const int inner_w = cw - static_cast<int>(2 * PAD);

    auto title_layout = create_heading_layout(ctx.cr, inner_w, 1);
    title_layout->set_markup(title_markup);
    int tw = 0, th = 0;
    title_layout->get_pixel_size(tw, th);

    std::string body_markup;
    if (title_attr != nullptr) {
        body_markup = build_inline_content(node);
    }
    auto body_layout = create_body_layout(ctx.cr, inner_w);
    body_layout->set_markup(body_markup);
    int bw = 0, bh = 0;
    body_layout->get_pixel_size(bw, bh);

    const double gap         = body_markup.empty() ? 0.0 : GAP;
    const double content_h   = PAD + th + gap + bh + PAD;
    const double total_h     = content_h < MIN_H ? MIN_H : content_h;

    // Dark overlay panel — matches web rgba(4,4,4,0.75) on black canvas.
    ctx.cr->set_source_rgba(rgb_r(::ase::colors::SURFACE_0_PANEL),
                            rgb_g(::ase::colors::SURFACE_0_PANEL),
                            rgb_b(::ase::colors::SURFACE_0_PANEL),
                            0.75);
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y, cw, total_h, RADIUS);
    ctx.cr->fill();

    ctx.cr->set_source_rgb(rgb_r(::ase::colors::CMS_HERO_TEXT),
                           rgb_g(::ase::colors::CMS_HERO_TEXT),
                           rgb_b(::ase::colors::CMS_HERO_TEXT));
    draw_layout(ctx.cr, title_layout, ctx.margin_left + TEXT_PAD, ctx.y + PAD);

    if (!body_markup.empty()) {
        ctx.cr->set_source_rgb(rgb_r(::ase::colors::CMS_HERO_TEXT),
                               rgb_g(::ase::colors::CMS_HERO_TEXT),
                               rgb_b(::ase::colors::CMS_HERO_TEXT));
        draw_layout(ctx.cr, body_layout, ctx.margin_left + TEXT_PAD, ctx.y + PAD + th + gap);
    }

    // my-6 = 24 px outer vertical margin (web wraps hero in my-6).
    return total_h + 24.0;
}

}  // namespace ase::viewer::render::dsl
