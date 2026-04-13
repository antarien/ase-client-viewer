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
    uint32_t total = count_children_named(node, "stat");
    int cols = (total > 0) ? static_cast<int>(total) : 3;
    if (cols < 1) cols = 1;
    const int col_w = cw / cols;
    const double h  = 76.0;

    ctx.cr->set_source_rgba(SURFACE_R, SURFACE_G, SURFACE_B, 0.5);
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y, cw, h, 4);
    ctx.cr->fill();

    const ase::markdown::Node* stat = first_child_named(node, "stat");
    int idx = 0;
    while (idx < cols) {
        const double cx = ctx.margin_left + idx * col_w;

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

        // Big value (chartreuse, larger heading).
        auto value_layout = create_heading_layout(ctx.cr, col_w, 1);
        auto value_fd = value_layout->get_font_description();
        value_fd.set_size(22 * Pango::SCALE);
        value_fd.set_weight(Pango::Weight::BOLD);
        value_layout->set_font_description(value_fd);
        std::string vmarkup;
        ::ase::viewer::render::append_escaped(vmarkup, value_str.c_str(),
                                              static_cast<uint32_t>(value_str.size()));
        value_layout->set_markup(vmarkup);
        int vw = 0, vh = 0;
        value_layout->get_pixel_size(vw, vh);
        ctx.cr->set_source_rgb(CYAN_R, CYAN_G, CYAN_B);
        draw_layout(ctx.cr, value_layout, cx + (col_w - vw) / 2.0, ctx.y + 12);

        // Muted label.
        auto label_layout = create_body_layout(ctx.cr, col_w);
        auto label_fd = label_layout->get_font_description();
        label_fd.set_size(9 * Pango::SCALE);
        label_layout->set_font_description(label_fd);
        label_layout->set_markup(label_str);
        int lw = 0, lh = 0;
        label_layout->get_pixel_size(lw, lh);
        ctx.cr->set_source_rgb(MUTED_R, MUTED_G, MUTED_B);
        draw_layout(ctx.cr, label_layout, cx + (col_w - lw) / 2.0,
                    ctx.y + 12 + vh + 4);

        if (stat != nullptr) stat = next_child_named(stat, "stat");
        ++idx;
    }

    return h + PARA_SPACING;
}

}  // namespace ase::viewer::render::dsl
