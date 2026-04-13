/**
 * DSL columns directive renderer.
 *
 * :::columns
 *   ::col
 *     left column markdown content
 *   ::col
 *     right column markdown content
 * :::
 *
 * Renders ::col children side-by-side with even widths. Each column
 * walks its own children with the inline markup builder so bold,
 * italic, code, NerdFont icons, links and inline math survive.
 *
 * @module      ase-client-viewer
 * @layer       5 (Clients)
 */

#include "vwr_dsl_cols.hpp"

namespace ase::viewer::render::dsl {

double render_columns(RenderContext& ctx, const ase::markdown::Node* node, int cw) {
    uint32_t col_total = count_children_named(node, "col");
    int col_count = (col_total > 0) ? static_cast<int>(col_total)
                                    : get_attr_int(node, "cols", 2);
    if (col_count < 1) col_count = 1;
    int col_w = cw / col_count;

    double max_h = 0.0;
    const ase::markdown::Node* col = first_child_named(node, "col");
    int idx = 0;

    while (idx < col_count) {
        const double x = ctx.margin_left + idx * col_w;

        std::string markup = (col != nullptr) ? build_inline_content(col) : std::string{};
        auto layout = create_body_layout(ctx.cr, col_w - 16);
        layout->set_markup(markup);
        int tw = 0, th = 0;
        layout->get_pixel_size(tw, th);

        const double col_h = th + 2 * BLOCK_PAD;

        ctx.cr->set_source_rgba(1.0, 1.0, 1.0, 0.015);
        ctx.cr->rectangle(x + 2, ctx.y, col_w - 4, col_h);
        ctx.cr->fill();

        ctx.cr->set_source_rgba(1.0, 1.0, 1.0, 0.04);
        ctx.cr->set_line_width(0.5);
        ctx.cr->rectangle(x + 2, ctx.y, col_w - 4, col_h);
        ctx.cr->stroke();

        ctx.cr->set_source_rgb(TEXT_R, TEXT_G, TEXT_B);
        draw_layout(ctx.cr, layout, x + 8, ctx.y + BLOCK_PAD);

        if (col_h > max_h) max_h = col_h;
        if (col != nullptr) col = next_child_named(col, "col");
        ++idx;
    }

    return max_h + PARA_SPACING;
}

}  // namespace ase::viewer::render::dsl
