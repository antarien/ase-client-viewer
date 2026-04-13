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
    double step_y  = ctx.y;
    double total_h = 0.0;
    int idx = 0;

    for (const ase::markdown::Node* step = first_child_named(node, "step"); step != nullptr;
         step = next_child_named(step, "step")) {
        const char* title_attr = get_attr(step, "title");

        std::string body_markup = build_inline_content(step);
        auto body_layout = create_body_layout(ctx.cr, cw - 50);
        body_layout->set_markup(body_markup);
        int bw = 0, bh = 0;
        body_layout->get_pixel_size(bw, bh);

        const double header_h = (title_attr != nullptr) ? 22.0 : 0.0;
        const double step_h   = header_h + bh + 16;

        // Numbered badge.
        const double bx = ctx.margin_left + 14.0;
        const double by = step_y + 14.0;
        ctx.cr->set_source_rgb(CYAN_R, CYAN_G, CYAN_B);
        ctx.cr->arc(bx, by, 12, 0, 2 * M_PI);
        ctx.cr->fill();

        std::string num_text = std::to_string(idx + 1);
        auto num_layout = create_body_layout(ctx.cr, 30);
        auto num_fd = num_layout->get_font_description();
        num_fd.set_weight(Pango::Weight::BOLD);
        num_layout->set_font_description(num_fd);
        num_layout->set_text(num_text);
        int nw = 0, nh = 0;
        num_layout->get_pixel_size(nw, nh);
        ctx.cr->set_source_rgb(0.02, 0.02, 0.02);
        draw_layout(ctx.cr, num_layout, bx - nw / 2.0, by - nh / 2.0);

        const double text_x = ctx.margin_left + 36;
        double text_y = step_y + 4;

        if (title_attr != nullptr) {
            std::string title_markup;
            ::ase::viewer::render::append_escaped(title_markup, title_attr);
            auto title_layout = create_body_layout(ctx.cr, cw - 50);
            auto fd = title_layout->get_font_description();
            fd.set_weight(Pango::Weight::BOLD);
            title_layout->set_font_description(fd);
            title_layout->set_markup(title_markup);
            ctx.cr->set_source_rgb(CYAN_R, CYAN_G, CYAN_B);
            draw_layout(ctx.cr, title_layout, text_x, text_y);
            text_y += header_h;
        }

        ctx.cr->set_source_rgb(TEXT_R, TEXT_G, TEXT_B);
        draw_layout(ctx.cr, body_layout, text_x, text_y);

        step_y  += step_h;
        total_h += step_h;
        ++idx;
    }

    return total_h + PARA_SPACING;
}

}  // namespace ase::viewer::render::dsl
