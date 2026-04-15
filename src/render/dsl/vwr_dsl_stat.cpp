/**
 * DSL stats directive renderer.
 *
 * :::stats
 *   ::stat{value="42" label="entities"}
 *   ::stat{value="1.2 ms" label="frame time"}
 *   ::stat{value="98%" label="cache hit"}
 * :::
 *
 * Renders a row of large-number metric tiles. Each ::stat shows a big
 * cyan value above a muted label. Falls back to inline text if the
 * value/label attributes are missing.
 *
 * @module      ase-client-viewer
 * @layer       5 (Clients)
 */

#include "vwr_dsl_stat.hpp"

#include "render/nerdfont_table.hpp"

namespace ase::viewer::render::dsl {

namespace {

uint32_t stat_color_for(const char* color) {
    if (color == nullptr) return ::ase::colors::CMS_STATS_ACCENT;
    if (::ase::viewer::render::streq(color, "cyan"))   return ::ase::colors::CMS_STATS_ACCENT;
    if (::ase::viewer::render::streq(color, "green"))  return ::ase::colors::PANEL_GREEN;
    if (::ase::viewer::render::streq(color, "orange")) return ::ase::colors::PANEL_ORANGE;
    if (::ase::viewer::render::streq(color, "purple")) return ::ase::colors::PANEL_PURPLE;
    if (::ase::viewer::render::streq(color, "red"))    return ::ase::colors::PANEL_RED;
    if (::ase::viewer::render::streq(color, "yellow")) return ::ase::colors::PANEL_YELLOW;
    return ::ase::colors::CMS_STATS_ACCENT;
}

}  // namespace

double render_stats(RenderContext& ctx, const ase::markdown::Node* node, int cw) {
    // Web 1:1 (cmsStatsBlock.tsx):
    //   container: my-4 (16 outer), grid gap-4 (16 between items)
    //   item: bg CMS_STATS_BG, border 1 px BORDER_A20, radius 4, p-4 (16),
    //         flex-col items-center justify-center text-center
    //   icon:  mb-2 (8), size-xl ≈ 24 px, color = color-attr mapped palette
    //   value: text-2xl (24) bold, same accent color
    //   label: text-xs (12) mt-1 (4), TEXT_PANEL_MUTED
    //
    // Cols count comes from `cols=N` attribute OR from actual ::stat
    // count whichever is smaller — web uses cols=N as a hint but actual
    // layout is a grid with min(N, children) columns.
    constexpr double PAD     = 16.0;
    constexpr double GAP     = 16.0;
    constexpr double RADIUS  = 4.0;
    constexpr int    ICON_PX = 24;
    constexpr double ICON_MB = 8.0;   // mb-2
    constexpr int    VAL_PX  = 24;    // text-2xl
    constexpr int    LBL_PX  = 12;    // text-xs
    constexpr double LBL_GAP = 4.0;   // mt-1

    const uint32_t total = count_children_named(node, "stat");
    if (total == 0) return 16.0;

    int cols = get_attr_int(node, "cols", static_cast<int>(total));
    if (cols < 1) cols = 1;
    if (cols > static_cast<int>(total)) cols = static_cast<int>(total);
    const int col_w = (cw - static_cast<int>(GAP) * (cols - 1)) / cols;

    // Single row for now — if `total > cols` we'd need multi-row, but
    // the demo always uses total == cols. Multi-row support can extend
    // this by looping the column layout per chunk of `cols` stats.
    //
    // Two-pass per row: pre-measure to find max_h, then draw uniformly.
    constexpr int MAX_STATS = 16;
    Glib::RefPtr<Pango::Layout> val_layouts[MAX_STATS];
    Glib::RefPtr<Pango::Layout> lbl_layouts[MAX_STATS];
    Glib::RefPtr<Pango::Layout> icon_layouts[MAX_STATS];
    uint32_t            accent[MAX_STATS] = {};
    int                 val_h[MAX_STATS]  = {};
    int                 lbl_h[MAX_STATS]  = {};
    int                 icn_h[MAX_STATS]  = {};
    bool                has_icon[MAX_STATS] = {};

    double max_content_h = 0.0;
    const ase::markdown::Node* stat = first_child_named(node, "stat");
    const int draw_count = (cols < MAX_STATS) ? cols : MAX_STATS;
    for (int idx = 0; idx < draw_count && stat != nullptr; ++idx) {
        const char* v_attr     = get_attr(stat, "value");
        const char* l_attr     = get_attr(stat, "label");
        const char* icon_attr  = get_attr(stat, "icon");
        const char* color_attr = get_attr(stat, "color");
        accent[idx] = stat_color_for(color_attr);

        // Icon layout (optional).
        if (icon_attr != nullptr) {
            const char* glyph = ::ase::viewer::render::lookup_nerdfont(icon_attr,
                ::ase::viewer::render::cstr_len(icon_attr));
            if (glyph != nullptr) {
                std::string im = "<span font_family=\"Symbols Nerd Font Mono\">";
                im.append(glyph);
                im.append("</span>");
                auto il = create_body_layout(ctx.cr, col_w);
                auto ifd = il->get_font_description();
                ifd.set_size(ICON_PX * Pango::SCALE);
                il->set_font_description(ifd);
                il->set_markup(im);
                int _iw = 0;
                il->get_pixel_size(_iw, icn_h[idx]);
                icon_layouts[idx] = il;
                has_icon[idx] = true;
            }
        }

        // Value layout.
        std::string value_str = (v_attr != nullptr) ? std::string{v_attr} : std::string{"—"};
        auto vl = create_heading_layout(ctx.cr, col_w, 1);
        auto vfd = vl->get_font_description();
        vfd.set_size(VAL_PX * Pango::SCALE);
        vfd.set_weight(Pango::Weight::BOLD);
        vl->set_font_description(vfd);
        std::string vm;
        ::ase::viewer::render::append_escaped(vm, value_str.c_str(),
                                              static_cast<uint32_t>(value_str.size()));
        vl->set_markup(vm);
        int _vw = 0;
        vl->get_pixel_size(_vw, val_h[idx]);
        val_layouts[idx] = vl;

        // Label layout.
        std::string label_str;
        if (l_attr != nullptr) {
            label_str = l_attr;
        } else {
            label_str = build_inline_content(stat);
            if (label_str.empty()) label_str = "metric";
        }
        auto ll = create_body_layout(ctx.cr, col_w - 2 * static_cast<int>(PAD));
        auto lfd = ll->get_font_description();
        lfd.set_size(LBL_PX * Pango::SCALE);
        ll->set_font_description(lfd);
        {
            std::string lm;
            ::ase::viewer::render::append_escaped(lm, label_str.c_str(),
                                                  static_cast<uint32_t>(label_str.size()));
            ll->set_markup(lm);
        }
        int _lw = 0;
        ll->get_pixel_size(_lw, lbl_h[idx]);
        lbl_layouts[idx] = ll;

        double content_h = val_h[idx] + LBL_GAP + lbl_h[idx];
        if (has_icon[idx]) content_h += icn_h[idx] + ICON_MB;
        if (content_h > max_content_h) max_content_h = content_h;

        stat = next_child_named(stat, "stat");
    }

    const double item_h = max_content_h + 2 * PAD;

    // Second pass: draw tiles uniform-height.
    for (int idx = 0; idx < draw_count; ++idx) {
        const double cx = ctx.margin_left + idx * (col_w + GAP);

        ctx.cr->set_source_rgb(rgb_r(::ase::colors::CMS_STATS_BG),
                               rgb_g(::ase::colors::CMS_STATS_BG),
                               rgb_b(::ase::colors::CMS_STATS_BG));
        rounded_rect(ctx.cr, cx, ctx.y, col_w, item_h, RADIUS);
        ctx.cr->fill();

        ctx.cr->set_source_rgba(rgb_r(::ase::colors::BORDER_A20),
                                rgb_g(::ase::colors::BORDER_A20),
                                rgb_b(::ase::colors::BORDER_A20),
                                rgb_a(::ase::colors::BORDER_A20));
        ctx.cr->set_line_width(1);
        rounded_rect(ctx.cr, cx, ctx.y, col_w, item_h, RADIUS);
        ctx.cr->stroke();

        // Vertical centering inside the tile (flex items-center justify-center).
        double content_h = val_h[idx] + LBL_GAP + lbl_h[idx];
        if (has_icon[idx]) content_h += icn_h[idx] + ICON_MB;
        double cy = ctx.y + (item_h - content_h) / 2.0;

        if (has_icon[idx] && icon_layouts[idx]) {
            int iw = 0, _ih = 0;
            icon_layouts[idx]->get_pixel_size(iw, _ih);
            ctx.cr->set_source_rgb(rgb_r(accent[idx]),
                                   rgb_g(accent[idx]),
                                   rgb_b(accent[idx]));
            draw_layout(ctx.cr, icon_layouts[idx], cx + (col_w - iw) / 2.0, cy);
            cy += icn_h[idx] + ICON_MB;
        }

        if (val_layouts[idx]) {
            int vw = 0, _vh = 0;
            val_layouts[idx]->get_pixel_size(vw, _vh);
            ctx.cr->set_source_rgb(rgb_r(accent[idx]),
                                   rgb_g(accent[idx]),
                                   rgb_b(accent[idx]));
            draw_layout(ctx.cr, val_layouts[idx], cx + (col_w - vw) / 2.0, cy);
            cy += val_h[idx] + LBL_GAP;
        }

        if (lbl_layouts[idx]) {
            int lw = 0, _lh = 0;
            lbl_layouts[idx]->get_pixel_size(lw, _lh);
            ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_PANEL_MUTED),
                                   rgb_g(::ase::colors::TEXT_PANEL_MUTED),
                                   rgb_b(::ase::colors::TEXT_PANEL_MUTED));
            draw_layout(ctx.cr, lbl_layouts[idx], cx + (col_w - lw) / 2.0, cy);
        }
    }

    return item_h + 16.0;  // my-4 outer
}

}  // namespace ase::viewer::render::dsl
