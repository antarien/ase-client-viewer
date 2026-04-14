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
    // Web 1:1 (cmsTabsBlock.tsx):
    //   container: my-4 (16), border 1 px BORDER_A20, radius 4
    //   header: bg CMS_TABS_HEADER_BG, border-bottom 1 px BORDER_A20
    //   button: px-4 py-2.5 (16/10), text-xs (12)
    //           active text TEXT_PANEL_WHITE, inactive TEXT_LABEL,
    //           active border-bottom 2 px (accent)
    //   content: bg CMS_TABS_BG, px-4 py-3 (16/12), text-sm (14),
    //            TEXT_PANEL_MUTED
    constexpr double BTN_PX        = 16.0;  // px-4
    constexpr double BTN_PY        = 10.0;  // py-2.5
    constexpr int    LABEL_PX      = 12;    // text-xs
    constexpr int    BODY_PX       = 14;    // text-sm
    constexpr double CONTENT_PX    = 16.0;  // px-4
    constexpr double CONTENT_PY    = 12.0;  // py-3
    constexpr double RADIUS        = 4.0;
    constexpr double ACTIVE_LINE   = 2.0;
    constexpr double STRIP_H       = LABEL_PX + 2 * BTN_PY;

    const uint32_t this_idx = ctx.next_tabs_index;
    ctx.next_tabs_index += 1;

    uint16_t active = 0;
    if (ctx.state != nullptr && this_idx < MAX_TABS_DIRECTIVES) {
        active = ctx.state->active_tab[this_idx];
    }

    // Header strip — CMS_TABS_HEADER_BG.
    ctx.cr->set_source_rgb(rgb_r(::ase::colors::CMS_TABS_HEADER_BG),
                           rgb_g(::ase::colors::CMS_TABS_HEADER_BG),
                           rgb_b(::ase::colors::CMS_TABS_HEADER_BG));
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y, cw, STRIP_H, RADIUS);
    ctx.cr->fill();

    double tab_x = ctx.margin_left;
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
        fd.set_size(LABEL_PX * Pango::SCALE);
        if (tab_idx == active) fd.set_weight(Pango::Weight::BOLD);
        label_layout->set_font_description(fd);
        label_layout->set_markup(label_markup);
        int lw = 0, lh = 0;
        label_layout->get_pixel_size(lw, lh);

        const double btn_w = lw + 2 * BTN_PX;

        if (tab_idx == active) {
            ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_PANEL_WHITE),
                                   rgb_g(::ase::colors::TEXT_PANEL_WHITE),
                                   rgb_b(::ase::colors::TEXT_PANEL_WHITE));
        } else {
            ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_LABEL),
                                   rgb_g(::ase::colors::TEXT_LABEL),
                                   rgb_b(::ase::colors::TEXT_LABEL));
        }
        draw_layout(ctx.cr, label_layout, tab_x + BTN_PX, ctx.y + BTN_PY);

        if (tab_idx == active) {
            // Border-bottom 2 px on active tab (web: accent color).
            ctx.cr->set_source_rgb(CYAN_R, CYAN_G, CYAN_B);
            ctx.cr->rectangle(tab_x, ctx.y + STRIP_H - ACTIVE_LINE, btn_w, ACTIVE_LINE);
            ctx.cr->fill();
        }

        // Hit region for this tab header (clickable area).
        if (ctx.regions != nullptr && ctx.regions->count < MAX_HIT_REGIONS) {
            auto& r = ctx.regions->items[ctx.regions->count++];
            r.x = tab_x;
            r.y = ctx.y;
            r.w = btn_w;
            r.h = STRIP_H;
            r.kind = HIT_KIND_TAB_HEADER;
            r.parent_idx = static_cast<uint16_t>(this_idx);
            r.child_idx  = static_cast<uint16_t>(tab_idx);
        }

        tab_x += btn_w;
        ++tab_idx;
    }

    if (active_tab == nullptr) active_tab = first_tab;

    double content_h = 0.0;
    if (active_tab != nullptr) {
        std::string body_markup = build_inline_content(active_tab);
        auto body_layout = create_body_layout(ctx.cr, cw - 2 * static_cast<int>(CONTENT_PX));
        auto body_fd = body_layout->get_font_description();
        body_fd.set_size(BODY_PX * Pango::SCALE);
        body_layout->set_font_description(body_fd);
        body_layout->set_markup(body_markup);
        int bw = 0, bh = 0;
        body_layout->get_pixel_size(bw, bh);

        const double content_y    = ctx.y + STRIP_H;
        const double content_full = bh + 2 * CONTENT_PY;

        // Content panel — CMS_TABS_BG.
        ctx.cr->set_source_rgb(rgb_r(::ase::colors::CMS_TABS_BG),
                               rgb_g(::ase::colors::CMS_TABS_BG),
                               rgb_b(::ase::colors::CMS_TABS_BG));
        ctx.cr->rectangle(ctx.margin_left, content_y, cw, content_full);
        ctx.cr->fill();

        ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_PANEL_MUTED),
                               rgb_g(::ase::colors::TEXT_PANEL_MUTED),
                               rgb_b(::ase::colors::TEXT_PANEL_MUTED));
        draw_layout(ctx.cr, body_layout, ctx.margin_left + CONTENT_PX, content_y + CONTENT_PY);

        content_h = content_full;
    }

    // Container border (1 px BORDER_A20).
    ctx.cr->set_source_rgba(rgb_r(::ase::colors::BORDER_A20),
                            rgb_g(::ase::colors::BORDER_A20),
                            rgb_b(::ase::colors::BORDER_A20),
                            rgb_a(::ase::colors::BORDER_A20));
    ctx.cr->set_line_width(1);
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y, cw, STRIP_H + content_h, RADIUS);
    ctx.cr->stroke();

    return STRIP_H + content_h + 16.0;  // my-4 outer
}

}  // namespace ase::viewer::render::dsl
