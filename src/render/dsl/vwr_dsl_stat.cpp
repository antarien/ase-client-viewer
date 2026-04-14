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

namespace ase::viewer::render::dsl {

double render_stats(RenderContext& ctx, const ase::markdown::Node* node, int cw) {
    // Web 1:1 (cmsStatsBlock.tsx):
    //   container: my-4 (16 outer), grid gap-4 (16 between items)
    //   item: bg CMS_STATS_BG, border 1 px BORDER_A20, radius 4, p-4 (16)
    //   value: text-2xl = 24 px bold, accent color
    //   label: text-xs = 12 px, mt-1 = 4, TEXT_PANEL_MUTED
    constexpr double PAD     = 16.0;
    constexpr double GAP     = 16.0;
    constexpr double RADIUS  = 4.0;
    constexpr int    VAL_PX  = 24;
    constexpr int    LBL_PX  = 12;
    constexpr double LBL_GAP = 4.0;

    uint32_t total = count_children_named(node, "stat");
    int cols = (total > 0) ? static_cast<int>(total) : 3;
    if (cols < 1) cols = 1;
    const int col_w = (cw - static_cast<int>(GAP) * (cols - 1)) / cols;

    // Pre-measure tallest content to size the row uniformly.
    double max_content_h = static_cast<double>(VAL_PX) + LBL_GAP + static_cast<double>(LBL_PX);
    const double item_h  = max_content_h + 2 * PAD;

    const ase::markdown::Node* stat = first_child_named(node, "stat");
    int idx = 0;
    while (idx < cols) {
        const double cx = ctx.margin_left + idx * (col_w + GAP);

        // Item background + border.
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

        std::string value_str = "—";
        std::string label_str = "metric";
        if (stat != nullptr) {
            const char* v_attr = get_attr(stat, "value");
            const char* l_attr = get_attr(stat, "label");
            if (v_attr != nullptr) value_str = v_attr;
            if (l_attr != nullptr) {
                label_str = l_attr;
            } else {
                std::string body = build_inline_content(stat);
                if (!body.empty()) label_str = body;
            }
        }

        // Big value — text-2xl bold, CMS_STATS_ACCENT color.
        auto value_layout = create_heading_layout(ctx.cr, col_w, 1);
        auto value_fd = value_layout->get_font_description();
        value_fd.set_size(VAL_PX * Pango::SCALE);
        value_fd.set_weight(Pango::Weight::BOLD);
        value_layout->set_font_description(value_fd);
        std::string vmarkup;
        ::ase::viewer::render::append_escaped(vmarkup, value_str.c_str(),
                                              static_cast<uint32_t>(value_str.size()));
        value_layout->set_markup(vmarkup);
        int vw = 0, vh = 0;
        value_layout->get_pixel_size(vw, vh);
        ctx.cr->set_source_rgb(rgb_r(::ase::colors::CMS_STATS_ACCENT),
                               rgb_g(::ase::colors::CMS_STATS_ACCENT),
                               rgb_b(::ase::colors::CMS_STATS_ACCENT));
        draw_layout(ctx.cr, value_layout, cx + (col_w - vw) / 2.0, ctx.y + PAD);

        // Label — text-xs, mt-1 (4 px) gap, TEXT_PANEL_MUTED.
        auto label_layout = create_body_layout(ctx.cr, col_w);
        auto label_fd = label_layout->get_font_description();
        label_fd.set_size(LBL_PX * Pango::SCALE);
        label_layout->set_font_description(label_fd);
        label_layout->set_markup(label_str);
        int lw = 0, lh = 0;
        label_layout->get_pixel_size(lw, lh);
        ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_PANEL_MUTED),
                               rgb_g(::ase::colors::TEXT_PANEL_MUTED),
                               rgb_b(::ase::colors::TEXT_PANEL_MUTED));
        draw_layout(ctx.cr, label_layout, cx + (col_w - lw) / 2.0,
                    ctx.y + PAD + vh + LBL_GAP);

        if (stat != nullptr) stat = next_child_named(stat, "stat");
        ++idx;
    }

    return item_h + 16.0;  // my-4 outer
}

}  // namespace ase::viewer::render::dsl
