/**
 * DSL steps directive renderer.
 *
 * :::steps
 *   ::step{title="Install dependencies"}
 *     run `npm install`
 *   ::step{title="Configure"}
 *     edit `config.toml`
 * :::
 *
 * Renders ::step children as a numbered checklist with circular badges
 * on the left and inline-formatted body content on the right. Step
 * bodies use the shared inline markup builder so bold/italic, code,
 * NerdFont icons and links survive.
 *
 * @module      ase-client-viewer
 * @layer       5 (Clients)
 */

#include "vwr_dsl_step.hpp"

namespace ase::viewer::render::dsl {

double render_steps(RenderContext& ctx, const ase::markdown::Node* node, int cw) {
    // Web 1:1 (cmsStepsBlock.tsx):
    //   container: my-4 (16 outer), pl-2 (8 left)
    //   item: min-h 60, gap-4 = 16 between circle and text
    //   circle: 28×28 round, bg CMS_STEPS_NUMBER, text SURFACE_0_INNER,
    //           text-xs (12) bold
    //   connector: 2 px wide, SURFACE_3
    //   content: pb-6 (24 bottom), pt-1 (4 top)
    //   title: text-sm (14) bold mb-1 (4), TEXT_PANEL_WHITE
    //   desc:  text-sm (14), TEXT_PANEL_MUTED
    constexpr double LEFT_PAD     = 8.0;
    constexpr double CIRCLE_DIA   = 28.0;
    constexpr double CIRCLE_R     = CIRCLE_DIA / 2.0;
    constexpr double GAP          = 16.0;
    constexpr double PB           = 24.0;
    constexpr double PT           = 4.0;
    constexpr double MIN_ITEM_H   = 60.0;
    constexpr int    NUM_PX       = 12;
    constexpr int    TXT_PX       = 14;
    constexpr double TITLE_MB     = 4.0;

    const double bx = ctx.margin_left + LEFT_PAD + CIRCLE_R;
    const double tx = ctx.margin_left + LEFT_PAD + CIRCLE_DIA + GAP;
    const int    text_w = cw - static_cast<int>(LEFT_PAD + CIRCLE_DIA + GAP);

    double step_y  = ctx.y;
    double total_h = 0.0;
    int idx = 0;

    for (const ase::markdown::Node* step = first_child_named(node, "step"); step != nullptr;
         step = next_child_named(step, "step")) {
        const char* title_attr = get_attr(step, "title");

        std::string body_markup = build_inline_content(step);
        auto body_layout = create_body_layout(ctx.cr, text_w);
        auto body_fd = body_layout->get_font_description();
        body_fd.set_size(TXT_PX * Pango::SCALE);
        body_layout->set_font_description(body_fd);
        body_layout->set_markup(body_markup);
        int bw = 0, bh = 0;
        body_layout->get_pixel_size(bw, bh);

        // Title height = text-sm + mb-1 (only when present).
        const double header_h = (title_attr != nullptr) ? (TXT_PX + TITLE_MB) : 0.0;
        double item_h = PT + header_h + bh + PB;
        if (item_h < MIN_ITEM_H) item_h = MIN_ITEM_H;

        // Connector line to next step (2 px, SURFACE_3).
        if (next_child_named(step, "step") != nullptr) {
            ctx.cr->set_source_rgb(rgb_r(::ase::colors::SURFACE_3),
                                   rgb_g(::ase::colors::SURFACE_3),
                                   rgb_b(::ase::colors::SURFACE_3));
            ctx.cr->set_line_width(2);
            ctx.cr->move_to(bx, step_y + CIRCLE_DIA);
            ctx.cr->line_to(bx, step_y + item_h);
            ctx.cr->stroke();
        }

        // Number circle — CMS_STEPS_NUMBER bg, SURFACE_0_INNER text.
        const double by = step_y + CIRCLE_R;
        ctx.cr->set_source_rgb(rgb_r(::ase::colors::CMS_STEPS_NUMBER),
                               rgb_g(::ase::colors::CMS_STEPS_NUMBER),
                               rgb_b(::ase::colors::CMS_STEPS_NUMBER));
        ctx.cr->arc(bx, by, CIRCLE_R, 0, 2 * M_PI);
        ctx.cr->fill();

        std::string num_text = std::to_string(idx + 1);
        auto num_layout = create_body_layout(ctx.cr, static_cast<int>(CIRCLE_DIA));
        auto num_fd = num_layout->get_font_description();
        num_fd.set_weight(Pango::Weight::BOLD);
        num_fd.set_size(NUM_PX * Pango::SCALE);
        num_layout->set_font_description(num_fd);
        num_layout->set_text(num_text);
        int nw = 0, nh = 0;
        num_layout->get_pixel_size(nw, nh);
        ctx.cr->set_source_rgb(rgb_r(::ase::colors::SURFACE_0_INNER),
                               rgb_g(::ase::colors::SURFACE_0_INNER),
                               rgb_b(::ase::colors::SURFACE_0_INNER));
        draw_layout(ctx.cr, num_layout, bx - nw / 2.0, by - nh / 2.0);

        double text_y = step_y + PT;

        if (title_attr != nullptr) {
            std::string title_markup;
            ::ase::viewer::render::append_escaped(title_markup, title_attr);
            auto title_layout = create_body_layout(ctx.cr, text_w);
            auto fd = title_layout->get_font_description();
            fd.set_weight(Pango::Weight::BOLD);
            fd.set_size(TXT_PX * Pango::SCALE);
            title_layout->set_font_description(fd);
            title_layout->set_markup(title_markup);
            ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_PANEL_WHITE),
                                   rgb_g(::ase::colors::TEXT_PANEL_WHITE),
                                   rgb_b(::ase::colors::TEXT_PANEL_WHITE));
            draw_layout(ctx.cr, title_layout, tx, text_y);
            text_y += header_h;
        }

        ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_PANEL_MUTED),
                               rgb_g(::ase::colors::TEXT_PANEL_MUTED),
                               rgb_b(::ase::colors::TEXT_PANEL_MUTED));
        draw_layout(ctx.cr, body_layout, tx, text_y);

        step_y  += item_h;
        total_h += item_h;
        ++idx;
    }

    return total_h + 16.0;  // my-4 outer
}

}  // namespace ase::viewer::render::dsl
