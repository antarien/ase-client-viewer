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
    // Web 1:1 (cmsTimelineBlock.tsx):
    //   container: my-4 (16), pl-2 (8 left)
    //   item: min-h 60, gap-4 = 16
    //   dot: 12×12 round, bg CMS_TIMELINE_DOT, mt-1 (4)
    //   connector: 2 px SURFACE_3
    //   content: pb-6 (24), TEXT_PANEL_WHITE
    //   date row: mb-1 (4), gap-2 (8) — inline
    //   date: text-xs (12), CMS_TIMELINE_DOT
    //   desc: text-sm (14), TEXT_PANEL_WHITE
    constexpr double LEFT_PAD   = 8.0;
    constexpr double DOT_DIA    = 12.0;
    constexpr double DOT_R      = DOT_DIA / 2.0;
    constexpr double DOT_MT     = 4.0;
    constexpr double GAP        = 16.0;
    constexpr double PB         = 24.0;
    constexpr int    DATE_PX    = 12;
    constexpr int    DESC_PX    = 14;
    constexpr double HDR_MB     = 4.0;

    if (count_children_named(node, "event") == 0) return 16.0;

    const double dx       = ctx.margin_left + LEFT_PAD + DOT_R;
    const double tx       = ctx.margin_left + LEFT_PAD + DOT_DIA + GAP;
    const int    text_w   = cw - static_cast<int>(LEFT_PAD + DOT_DIA + GAP);

    double event_y = ctx.y;
    double total_h = 0.0;

    for (const ase::markdown::Node* event = first_child_named(node, "event"); event != nullptr;
         event = next_child_named(event, "event")) {
        const char* date_attr  = get_attr(event, "date");
        const char* title_attr = get_attr(event, "title");

        std::string body_markup = build_inline_content(event);
        auto body_layout = create_body_layout(ctx.cr, text_w);
        auto body_fd = body_layout->get_font_description();
        body_fd.set_size(DESC_PX * Pango::SCALE);
        body_layout->set_font_description(body_fd);
        body_layout->set_markup(body_markup);
        int bw = 0, bh = 0;
        body_layout->get_pixel_size(bw, bh);

        const double header_h = (date_attr != nullptr || title_attr != nullptr)
                                    ? (DATE_PX + HDR_MB)
                                    : 0.0;
        const double event_h  = header_h + bh + PB;

        // Connector line to next event.
        if (next_child_named(event, "event") != nullptr) {
            ctx.cr->set_source_rgb(rgb_r(::ase::colors::SURFACE_3),
                                   rgb_g(::ase::colors::SURFACE_3),
                                   rgb_b(::ase::colors::SURFACE_3));
            ctx.cr->set_line_width(2);
            ctx.cr->move_to(dx, event_y + DOT_MT + DOT_DIA);
            ctx.cr->line_to(dx, event_y + event_h);
            ctx.cr->stroke();
        }

        // Marker dot — CMS_TIMELINE_DOT, mt-1 offset.
        ctx.cr->set_source_rgb(rgb_r(::ase::colors::CMS_TIMELINE_DOT),
                               rgb_g(::ase::colors::CMS_TIMELINE_DOT),
                               rgb_b(::ase::colors::CMS_TIMELINE_DOT));
        ctx.cr->arc(dx, event_y + DOT_MT + DOT_R, DOT_R, 0, 2 * M_PI);
        ctx.cr->fill();

        double text_y = event_y;

        if (date_attr != nullptr || title_attr != nullptr) {
            std::string header_markup;
            if (date_attr != nullptr) {
                header_markup.append("<span foreground=\"");
                header_markup.append(color_hex(::ase::colors::CMS_TIMELINE_DOT));
                header_markup.append("\">");
                ::ase::viewer::render::append_escaped(header_markup, date_attr);
                header_markup.append("</span>");
                if (title_attr != nullptr) header_markup.append("  ");
            }
            if (title_attr != nullptr) {
                header_markup.append("<b>");
                ::ase::viewer::render::append_escaped(header_markup, title_attr);
                header_markup.append("</b>");
            }
            auto header_layout = create_body_layout(ctx.cr, text_w);
            auto fd = header_layout->get_font_description();
            fd.set_weight(Pango::Weight::BOLD);
            fd.set_size(DATE_PX * Pango::SCALE);
            header_layout->set_font_description(fd);
            header_layout->set_markup(header_markup);
            ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_PANEL_WHITE),
                                   rgb_g(::ase::colors::TEXT_PANEL_WHITE),
                                   rgb_b(::ase::colors::TEXT_PANEL_WHITE));
            draw_layout(ctx.cr, header_layout, tx, text_y);
            text_y += header_h;
        }

        ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_PANEL_WHITE),
                               rgb_g(::ase::colors::TEXT_PANEL_WHITE),
                               rgb_b(::ase::colors::TEXT_PANEL_WHITE));
        draw_layout(ctx.cr, body_layout, tx, text_y);

        event_y += event_h;
        total_h += event_h;
    }

    return total_h + 16.0;  // my-4 outer
}

}  // namespace ase::viewer::render::dsl
