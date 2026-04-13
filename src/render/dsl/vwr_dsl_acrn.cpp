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
    constexpr double HEADER_H = 30.0;

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

        ctx.cr->set_source_rgba(1.0, 1.0, 1.0, 0.03);
        ctx.cr->rectangle(ctx.margin_left, header_y, cw, HEADER_H);
        ctx.cr->fill();
        ctx.cr->set_source_rgba(1.0, 1.0, 1.0, 0.06);
        ctx.cr->set_line_width(0.5);
        ctx.cr->move_to(ctx.margin_left, header_y + HEADER_H);
        ctx.cr->line_to(ctx.margin_left + cw, header_y + HEADER_H);
        ctx.cr->stroke();

        std::string title_markup = "<span foreground=\"#80deff\">";
        title_markup.append(is_open ? "▼" : "▶");
        title_markup.append(" </span>");
        if (title_attr != nullptr) {
            ::ase::viewer::render::append_escaped(title_markup, title_attr);
        } else {
            title_markup.append("Section");
        }

        auto title_layout = create_body_layout(ctx.cr, cw - 24);
        auto fd = title_layout->get_font_description();
        fd.set_weight(Pango::Weight::BOLD);
        title_layout->set_font_description(fd);
        title_layout->set_markup(title_markup);
        int hlw = 0, hlh = 0;
        title_layout->get_pixel_size(hlw, hlh);

        ctx.cr->set_source_rgb(CYAN_R, CYAN_G, CYAN_B);
        draw_layout(ctx.cr, title_layout, ctx.margin_left + 10, header_y + (HEADER_H - hlh) / 2.0);

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
            auto body_layout = create_body_layout(ctx.cr, cw - 32);
            body_layout->set_markup(body_markup);
            int bw = 0, bh = 0;
            body_layout->get_pixel_size(bw, bh);

            ctx.cr->set_source_rgba(1.0, 1.0, 1.0, 0.015);
            ctx.cr->rectangle(ctx.margin_left, ctx.y + total_h, cw, bh + 2 * BLOCK_PAD);
            ctx.cr->fill();

            ctx.cr->set_source_rgb(TEXT_R, TEXT_G, TEXT_B);
            draw_layout(ctx.cr, body_layout, ctx.margin_left + 16, ctx.y + total_h + BLOCK_PAD);

            total_h += bh + 2 * BLOCK_PAD;
        }

        total_h += 2;
        ++panel_idx;
    }

    return total_h + PARA_SPACING;
}

}  // namespace ase::viewer::render::dsl
