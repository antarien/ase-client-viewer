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
    // Web 1:1 (cmsColumnsBlock.tsx):
    //   container: mt-6 mb-4 (24/16 outer), grid gap-4 (16 between cols)
    //   col cell: bg CMS_COLUMN_BG (#0c0c0c), border 1 px BORDER_A20,
    //             radius 4, p-4 (16 padding all sides)
    //   text: TEXT_PANEL_WHITE, text-sm (14 px)
    constexpr double PAD     = 16.0;
    constexpr double GAP     = 16.0;
    constexpr double RADIUS  = 4.0;

    uint32_t col_total = count_children_named(node, "col");
    int col_count = (col_total > 0) ? static_cast<int>(col_total)
                                    : get_attr_int(node, "cols", 2);
    if (col_count < 1) col_count = 1;
    const int col_w = (cw - static_cast<int>(GAP) * (col_count - 1)) / col_count;

    // Two-pass: pre-measure every column so every cell gets the uniform
    // max_h (CSS grid row stretches to the tallest child). Drawing each
    // column with its own height leaves uneven backgrounds next to each
    // other.
    constexpr int MAX_COLS = 8;
    Glib::RefPtr<Pango::Layout> layouts[MAX_COLS];
    double heights[MAX_COLS] = {};
    const ase::markdown::Node* col_nodes[MAX_COLS] = {};

    double max_h = 0.0;
    const ase::markdown::Node* col = first_child_named(node, "col");
    const int draw_count = (col_count < MAX_COLS) ? col_count : MAX_COLS;
    for (int idx = 0; idx < draw_count; ++idx) {
        col_nodes[idx] = col;
        std::string markup = (col != nullptr) ? build_inline_content(col) : std::string{};
        auto layout = create_body_layout(ctx.cr, col_w - 2 * static_cast<int>(PAD));
        layout->set_markup(markup);
        int tw = 0, th = 0;
        layout->get_pixel_size(tw, th);
        layouts[idx] = layout;
        heights[idx] = th + 2 * PAD;
        if (heights[idx] > max_h) max_h = heights[idx];
        if (col != nullptr) col = next_child_named(col, "col");
    }

    for (int idx = 0; idx < draw_count; ++idx) {
        const double x = ctx.margin_left + idx * (col_w + GAP);

        ctx.cr->set_source_rgb(rgb_r(::ase::colors::CMS_COLUMN_BG),
                               rgb_g(::ase::colors::CMS_COLUMN_BG),
                               rgb_b(::ase::colors::CMS_COLUMN_BG));
        rounded_rect(ctx.cr, x, ctx.y, col_w, max_h, RADIUS);
        ctx.cr->fill();

        ctx.cr->set_source_rgba(rgb_r(::ase::colors::BORDER_A20),
                                rgb_g(::ase::colors::BORDER_A20),
                                rgb_b(::ase::colors::BORDER_A20),
                                rgb_a(::ase::colors::BORDER_A20));
        ctx.cr->set_line_width(1);
        rounded_rect(ctx.cr, x, ctx.y, col_w, max_h, RADIUS);
        ctx.cr->stroke();

        if (layouts[idx]) {
            ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_PANEL_WHITE),
                                   rgb_g(::ase::colors::TEXT_PANEL_WHITE),
                                   rgb_b(::ase::colors::TEXT_PANEL_WHITE));
            draw_layout(ctx.cr, layouts[idx], x + PAD, ctx.y + PAD);
        }
    }

    // mt-6 mb-4 = 24 + 16 outer.
    return max_h + 24.0 + 16.0;
}

}  // namespace ase::viewer::render::dsl
