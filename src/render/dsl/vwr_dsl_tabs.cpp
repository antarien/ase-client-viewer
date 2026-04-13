/**
 * DSL tabs directive renderer with click-to-switch interactivity.
 *
 * :::tabs
 *   ::tab{title="Overview"}
 *     content for the first tab
 *   ::tab{title="Detail"}
 *     content for the second tab
 * :::
 *
 * Each :::tabs directive consumes a sequential index (ctx.next_tabs_index)
 * which serves as the key into ctx.state->active_tab. Clicking a tab
 * header writes a HitRegion into ctx.regions; the viewer's GestureClick
 * handler updates the active index and queues a redraw. Tabs walk their
 * children with the inline markup builder so headings, bold/italic, code,
 * NerdFont icons and inline math survive in the active tab body.
 *
 * @module      ase-client-viewer
 * @layer       5 (Clients)
 */

#include "vwr_dsl_tabs.hpp"

namespace ase::viewer::render::dsl {

double render_tabs(RenderContext& ctx, const ase::markdown::Node* node, int cw) {
    constexpr double STRIP_H = 30.0;

    const uint32_t this_idx = ctx.next_tabs_index;
    ctx.next_tabs_index += 1;

    uint16_t active = 0;
    if (ctx.state != nullptr && this_idx < MAX_TABS_DIRECTIVES) {
        active = ctx.state->active_tab[this_idx];
    }

    // Strip background.
    ctx.cr->set_source_rgba(1.0, 1.0, 1.0, 0.03);
    ctx.cr->rectangle(ctx.margin_left, ctx.y, cw, STRIP_H);
    ctx.cr->fill();

    double tab_x = ctx.margin_left + 8.0;
    int tab_idx = 0;
    const ase::markdown::Node* active_tab = nullptr;
    const ase::markdown::Node* first_tab  = nullptr;

    for (const ase::markdown::Node* tab = first_child_named(node, "tab"); tab != nullptr;
         tab = next_child_named(tab, "tab")) {
        if (first_tab == nullptr) first_tab = tab;
        if (tab_idx == active) active_tab = tab;

        const char* title_attr = get_attr(tab, "title");
        if (title_attr == nullptr) title_attr = get_attr(tab, "label");

        std::string label_markup;
        if (title_attr != nullptr) {
            ::ase::viewer::render::append_escaped(label_markup, title_attr);
        } else {
            label_markup = std::string{"Tab "} + std::to_string(tab_idx + 1);
        }

        auto label_layout = create_body_layout(ctx.cr, cw);
        auto fd = label_layout->get_font_description();
        if (tab_idx == active) fd.set_weight(Pango::Weight::BOLD);
        label_layout->set_font_description(fd);
        label_layout->set_markup(label_markup);
        int lw = 0, lh = 0;
        label_layout->get_pixel_size(lw, lh);

        if (tab_idx == active) {
            ctx.cr->set_source_rgb(CYAN_R, CYAN_G, CYAN_B);
        } else {
            ctx.cr->set_source_rgb(MUTED_R, MUTED_G, MUTED_B);
        }
        draw_layout(ctx.cr, label_layout, tab_x, ctx.y + (STRIP_H - lh) / 2.0);

        if (tab_idx == active) {
            ctx.cr->set_source_rgb(CYAN_R, CYAN_G, CYAN_B);
            ctx.cr->rectangle(tab_x - 4, ctx.y + STRIP_H - 2, lw + 8, 2);
            ctx.cr->fill();
        }

        // Hit region for this tab header (clickable area).
        if (ctx.regions != nullptr && ctx.regions->count < MAX_HIT_REGIONS) {
            auto& r = ctx.regions->items[ctx.regions->count++];
            r.x = tab_x - 4;
            r.y = ctx.y;
            r.w = lw + 8;
            r.h = STRIP_H;
            r.kind = HIT_KIND_TAB_HEADER;
            r.parent_idx = static_cast<uint16_t>(this_idx);
            r.child_idx  = static_cast<uint16_t>(tab_idx);
        }

        tab_x += lw + 24;
        ++tab_idx;
    }

    if (active_tab == nullptr) active_tab = first_tab;

    double content_h = 0.0;
    if (active_tab != nullptr) {
        std::string body_markup = build_inline_content(active_tab);
        auto body_layout = create_body_layout(ctx.cr, cw - 24);
        body_layout->set_markup(body_markup);
        int bw = 0, bh = 0;
        body_layout->get_pixel_size(bw, bh);

        const double content_y = ctx.y + STRIP_H + 4;
        ctx.cr->set_source_rgba(1.0, 1.0, 1.0, 0.02);
        ctx.cr->rectangle(ctx.margin_left, content_y, cw, bh + 2 * BLOCK_PAD);
        ctx.cr->fill();

        ctx.cr->set_source_rgb(TEXT_R, TEXT_G, TEXT_B);
        draw_layout(ctx.cr, body_layout, ctx.margin_left + 12, content_y + BLOCK_PAD);

        content_h = bh + 2 * BLOCK_PAD + 4;
    }

    return STRIP_H + content_h + PARA_SPACING;
}

}  // namespace ase::viewer::render::dsl
