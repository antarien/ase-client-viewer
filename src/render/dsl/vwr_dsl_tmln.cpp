/**
 * DSL timeline directive renderer.
 *
 * :::timeline
 *   ::event{date="2026-04-12" title="Phase A complete"}
 *     event description with **bold** and *italic* and (nf-fa-rocket)
 *   ::event{date="2026-04-13"}
 *     more event content
 * :::
 *
 * Renders ::event children as a vertical timeline with a connecting line
 * between bullet markers. Each event walks its own children with the
 * inline markup builder so dates, titles and bodies all preserve
 * inline structure.
 *
 * @module      ase-client-viewer
 * @layer       5 (Clients)
 */

#include "vwr_dsl_tmln.hpp"

namespace ase::viewer::render::dsl {

double render_timeline(RenderContext& ctx, const ase::markdown::Node* node, int cw) {
    if (count_children_named(node, "event") == 0) return PARA_SPACING;

    const double cx = ctx.margin_left + 14.0;
    double event_y  = ctx.y;
    double total_h  = 0.0;

    for (const ase::markdown::Node* event = first_child_named(node, "event"); event != nullptr;
         event = next_child_named(event, "event")) {
        const char* date_attr  = get_attr(event, "date");
        const char* title_attr = get_attr(event, "title");

        std::string body_markup = build_inline_content(event);
        auto body_layout = create_body_layout(ctx.cr, cw - 50);
        body_layout->set_markup(body_markup);
        int bw = 0, bh = 0;
        body_layout->get_pixel_size(bw, bh);

        const double header_h = (date_attr != nullptr || title_attr != nullptr) ? 18.0 : 0.0;
        const double event_h  = header_h + bh + 12;

        // Connector line to next event.
        if (next_child_named(event, "event") != nullptr) {
            ctx.cr->set_source_rgba(CYAN_R, CYAN_G, CYAN_B, 0.25);
            ctx.cr->set_line_width(2);
            ctx.cr->move_to(cx, event_y + 8);
            ctx.cr->line_to(cx, event_y + event_h);
            ctx.cr->stroke();
        }

        // Marker dot.
        ctx.cr->set_source_rgb(CYAN_R, CYAN_G, CYAN_B);
        ctx.cr->arc(cx, event_y + 8, 5, 0, 2 * M_PI);
        ctx.cr->fill();

        const double text_x = cx + 20;
        double text_y = event_y;

        if (date_attr != nullptr || title_attr != nullptr) {
            std::string header_markup;
            if (date_attr != nullptr) {
                header_markup.append("<span foreground=\"#80deff\">");
                ::ase::viewer::render::append_escaped(header_markup, date_attr);
                header_markup.append("</span>");
                if (title_attr != nullptr) header_markup.append("  ");
            }
            if (title_attr != nullptr) {
                header_markup.append("<b>");
                ::ase::viewer::render::append_escaped(header_markup, title_attr);
                header_markup.append("</b>");
            }
            auto header_layout = create_body_layout(ctx.cr, cw - 50);
            auto fd = header_layout->get_font_description();
            fd.set_weight(Pango::Weight::BOLD);
            header_layout->set_font_description(fd);
            header_layout->set_markup(header_markup);
            ctx.cr->set_source_rgb(CYAN_R, CYAN_G, CYAN_B);
            draw_layout(ctx.cr, header_layout, text_x, text_y);
            text_y += header_h;
        }

        ctx.cr->set_source_rgb(TEXT_R, TEXT_G, TEXT_B);
        draw_layout(ctx.cr, body_layout, text_x, text_y);

        event_y += event_h;
        total_h += event_h;
    }

    return total_h + PARA_SPACING;
}

}  // namespace ase::viewer::render::dsl
