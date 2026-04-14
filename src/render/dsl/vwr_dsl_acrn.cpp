/**
 * DSL accordion directive renderer with click-to-expand interactivity.
 *
 * :::accordion
 *   ::panel{title="Section 1" open=true}
 *     content for section 1
 *   ::panel{title="Section 2"}
 *     content for section 2
 * :::
 *
 * Each :::accordion consumes a sequential index (ctx.next_accordion_index).
 * On the first traversal the open state is initialised from the panel
 * `open=true` attributes (or the first panel only when none are flagged).
 * Subsequent traversals read the persisted open state from
 * ctx.state->panel_open[idx][...]. Header bars become click regions; the
 * viewer toggles the matching bit on click and queues a redraw.
 *
 * @module      ase-client-viewer
 * @layer       5 (Clients)
 */

#include "vwr_dsl_acrn.hpp"

namespace ase::viewer::render::dsl {

double render_accordion(RenderContext& ctx, const ase::markdown::Node* node, int cw) {
    // Web 1:1 (cmsAccordionBlock.tsx):
    //   container: my-4 (16 outer), border 1 px BORDER_A20, radius 4
    //   header: px-4 py-3 (16/12), text-xs (12), TEXT_PANEL_WHITE
    //           bg open=CMS_ACCORDION_HEADER_BG, closed=CMS_ACCORDION_BG
    //   content: px-4 py-3 (16/12), text-2xs (10), TEXT_PANEL_MUTED,
    //            bg CMS_ACCORDION_BG
    //   panel divider: border-bottom 1 px BORDER_A20
    constexpr double HDR_PX     = 16.0;  // px-4
    constexpr double HDR_PY     = 12.0;  // py-3
    constexpr double BODY_PX    = 16.0;  // px-4
    constexpr double BODY_PY    = 12.0;  // py-3
    constexpr int    HDR_TXT_PX = 12;    // text-xs
    constexpr int    BODY_TXT_PX = 10;   // text-2xs
    constexpr double RADIUS     = 4.0;
    constexpr double HEADER_H   = HDR_TXT_PX + 2 * HDR_PY;

    const uint32_t this_idx = ctx.next_accordion_index;
    ctx.next_accordion_index += 1;

    InteractiveState* state = ctx.state;

    // First-time initialisation — seed open flags from the directive
    // attributes so the user sees the same default the doc author wrote.
    if (state != nullptr && this_idx < MAX_ACCORDIONS && !state->panel_initialized[this_idx]) {
        state->panel_initialized[this_idx] = true;
        bool any_open = false;
        uint16_t pi = 0;
        for (const ase::markdown::Node* panel = first_child_named(node, "panel"); panel != nullptr;
             panel = next_child_named(panel, "panel")) {
            const char* open_attr = get_attr(panel, "open");
            if (open_attr != nullptr && ::ase::viewer::render::streq(open_attr, "true")) {
                if (pi < MAX_PANELS_PER_ACRN) state->panel_open[this_idx][pi] = 1;
                any_open = true;
            }
            ++pi;
        }
        if (!any_open && pi > 0 && 0 < MAX_PANELS_PER_ACRN) {
            state->panel_open[this_idx][0] = 1;
        }
    }

    double total_h = 0.0;
    uint16_t panel_idx = 0;

    for (const ase::markdown::Node* panel = first_child_named(node, "panel"); panel != nullptr;
         panel = next_child_named(panel, "panel")) {
        bool is_open = false;
        if (state != nullptr && this_idx < MAX_ACCORDIONS && panel_idx < MAX_PANELS_PER_ACRN) {
            is_open = state->panel_open[this_idx][panel_idx] != 0;
        } else if (panel_idx == 0) {
            is_open = true;
        }

        const char* title_attr = get_attr(panel, "title");
        const double header_y = ctx.y + total_h;

        // Header background — open vs closed.
        const uint32_t hdr_argb = is_open
            ? ::ase::colors::CMS_ACCORDION_HEADER_BG
            : ::ase::colors::CMS_ACCORDION_BG;
        ctx.cr->set_source_rgb(rgb_r(hdr_argb), rgb_g(hdr_argb), rgb_b(hdr_argb));
        ctx.cr->rectangle(ctx.margin_left, header_y, cw, HEADER_H);
        ctx.cr->fill();

        // Panel divider — border-bottom 1 px BORDER_A20.
        ctx.cr->set_source_rgba(rgb_r(::ase::colors::BORDER_A20),
                                rgb_g(::ase::colors::BORDER_A20),
                                rgb_b(::ase::colors::BORDER_A20),
                                rgb_a(::ase::colors::BORDER_A20));
        ctx.cr->set_line_width(1);
        ctx.cr->move_to(ctx.margin_left, header_y + HEADER_H);
        ctx.cr->line_to(ctx.margin_left + cw, header_y + HEADER_H);
        ctx.cr->stroke();

        std::string title_markup = "<span foreground=\"";
        title_markup.append(color_hex(::ase::colors::CMS_ACCORDION_ICON));
        title_markup.append("\">");
        title_markup.append(is_open ? "▼" : "▶");
        title_markup.append(" </span>");
        if (title_attr != nullptr) {
            ::ase::viewer::render::append_escaped(title_markup, title_attr);
        } else {
            title_markup.append("Section");
        }

        auto title_layout = create_body_layout(ctx.cr, cw - 2 * static_cast<int>(HDR_PX));
        auto fd = title_layout->get_font_description();
        fd.set_weight(Pango::Weight::BOLD);
        fd.set_size(HDR_TXT_PX * Pango::SCALE);
        title_layout->set_font_description(fd);
        title_layout->set_markup(title_markup);
        int hlw = 0, hlh = 0;
        title_layout->get_pixel_size(hlw, hlh);

        ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_PANEL_WHITE),
                               rgb_g(::ase::colors::TEXT_PANEL_WHITE),
                               rgb_b(::ase::colors::TEXT_PANEL_WHITE));
        draw_layout(ctx.cr, title_layout, ctx.margin_left + HDR_PX, header_y + (HEADER_H - hlh) / 2.0);

        // Hit region for clicking the header bar.
        if (ctx.regions != nullptr && ctx.regions->count < MAX_HIT_REGIONS) {
            auto& r = ctx.regions->items[ctx.regions->count++];
            r.x = ctx.margin_left;
            r.y = header_y;
            r.w = cw;
            r.h = HEADER_H;
            r.kind = HIT_KIND_PANEL_HEADER;
            r.parent_idx = static_cast<uint16_t>(this_idx);
            r.child_idx  = panel_idx;
        }

        total_h += HEADER_H;

        if (is_open) {
            std::string body_markup = build_inline_content(panel);
            auto body_layout = create_body_layout(ctx.cr, cw - 2 * static_cast<int>(BODY_PX));
            auto body_fd = body_layout->get_font_description();
            body_fd.set_size(BODY_TXT_PX * Pango::SCALE);
            body_layout->set_font_description(body_fd);
            body_layout->set_markup(body_markup);
            int bw = 0, bh = 0;
            body_layout->get_pixel_size(bw, bh);

            const double body_full = bh + 2 * BODY_PY;

            ctx.cr->set_source_rgb(rgb_r(::ase::colors::CMS_ACCORDION_BG),
                                   rgb_g(::ase::colors::CMS_ACCORDION_BG),
                                   rgb_b(::ase::colors::CMS_ACCORDION_BG));
            ctx.cr->rectangle(ctx.margin_left, ctx.y + total_h, cw, body_full);
            ctx.cr->fill();

            ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_PANEL_MUTED),
                                   rgb_g(::ase::colors::TEXT_PANEL_MUTED),
                                   rgb_b(::ase::colors::TEXT_PANEL_MUTED));
            draw_layout(ctx.cr, body_layout, ctx.margin_left + BODY_PX, ctx.y + total_h + BODY_PY);

            total_h += body_full;
        }

        ++panel_idx;
    }

    // Container outline border (1 px BORDER_A20).
    ctx.cr->set_source_rgba(rgb_r(::ase::colors::BORDER_A20),
                            rgb_g(::ase::colors::BORDER_A20),
                            rgb_b(::ase::colors::BORDER_A20),
                            rgb_a(::ase::colors::BORDER_A20));
    ctx.cr->set_line_width(1);
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y, cw, total_h, RADIUS);
    ctx.cr->stroke();

    return total_h + 16.0;  // my-4 outer
}

}  // namespace ase::viewer::render::dsl
