/**
 * DSL cards directive renderer.
 *
 * :::cards{cols=3}
 *   ::card{title="…" icon="nf-fa-…"}
 *     body markdown content
 *   ::card{title="…"}
 *     more body content
 * :::
 *
 * Lays out child ::card directives into a CSS-grid-style row of N
 * columns (default 3). Each card walks its own children for inline body
 * content, supports a title attribute and optional icon. Heights are
 * measured per card so the row aligns to the tallest entry.
 *
 * @module      ase-client-viewer
 * @layer       5 (Clients)
 */

#include "vwr_dsl_card.hpp"

#include "render/nerdfont_table.hpp"

namespace ase::viewer::render::dsl {

using ::ase::viewer::render::lookup_nerdfont;

namespace {

double measure_and_render_card(RenderContext& ctx,
                               const ase::markdown::Node* card,
                               double x, double y, double w,
                               bool measure_only) {
    const char* title_attr = get_attr(card, "title");
    const char* icon_attr  = get_attr(card, "icon");

    std::string body_markup = build_inline_content(card);

    auto body_layout = create_body_layout(ctx.cr, static_cast<int>(w) - 24);
    body_layout->set_markup(body_markup);
    int bw = 0, bh = 0;
    body_layout->get_pixel_size(bw, bh);

    const double title_h = (title_attr != nullptr) ? 24.0 : 0.0;
    const double card_h  = title_h + bh + 2 * BLOCK_PAD;

    if (measure_only) return card_h;

    ctx.cr->set_source_rgb(SURFACE_R, SURFACE_G, SURFACE_B);
    rounded_rect(ctx.cr, x + 4, y, w - 8, card_h, 4);
    ctx.cr->fill();

    ctx.cr->set_source_rgba(CYAN_R, CYAN_G, CYAN_B, 0.18);
    ctx.cr->set_line_width(1);
    rounded_rect(ctx.cr, x + 4, y, w - 8, card_h, 4);
    ctx.cr->stroke();

    double cy = y + BLOCK_PAD;

    if (title_attr != nullptr) {
        std::string title_markup;
        if (icon_attr != nullptr) {
            const char* glyph = lookup_nerdfont(icon_attr,
                                                ::ase::viewer::render::cstr_len(icon_attr));
            if (glyph != nullptr) {
                title_markup.append("<span font_family=\"Symbols Nerd Font Mono\">");
                title_markup.append(glyph);
                title_markup.append("</span>  ");
            }
        }
        ::ase::viewer::render::append_escaped(title_markup, title_attr);

        auto title_layout = create_body_layout(ctx.cr, static_cast<int>(w) - 24);
        auto fd = title_layout->get_font_description();
        fd.set_weight(Pango::Weight::BOLD);
        title_layout->set_font_description(fd);
        title_layout->set_markup(title_markup);

        ctx.cr->set_source_rgb(CYAN_R, CYAN_G, CYAN_B);
        draw_layout(ctx.cr, title_layout, x + 12, cy);
        cy += 22;
    }

    ctx.cr->set_source_rgb(TEXT_R, TEXT_G, TEXT_B);
    draw_layout(ctx.cr, body_layout, x + 12, cy);

    return card_h;
}

}  // namespace

double render_cards(RenderContext& ctx, const ase::markdown::Node* node, int cw) {
    int total = static_cast<int>(count_children_named(node, "card"));
    if (total == 0) {
        // Fallback: render the directive itself as a single full-width card.
        return measure_and_render_card(ctx, node, ctx.margin_left, ctx.y, cw, false) + PARA_SPACING;
    }

    int cols = get_attr_int(node, "cols", 3);
    if (cols < 1) cols = 1;
    if (cols > total) cols = total;
    int card_w = cw / cols;

    double row_y = ctx.y;
    int idx = 0;
    double total_h = 0.0;

    while (idx < total) {
        // Walk this row's cards once to measure max height.
        double max_h = 0.0;
        int row_count = 0;
        const ase::markdown::Node* iter = first_child_named(node, "card");
        for (int skip = 0; iter != nullptr && skip < idx; ++skip) {
            iter = next_child_named(iter, "card");
        }
        const ase::markdown::Node* row_first = iter;
        while (iter != nullptr && row_count < cols) {
            double h = measure_and_render_card(ctx, iter, 0, 0,
                                               static_cast<double>(card_w), true);
            if (h > max_h) max_h = h;
            iter = next_child_named(iter, "card");
            ++row_count;
        }

        // Draw this row at the actual coordinates.
        iter = row_first;
        for (int col = 0; col < row_count && iter != nullptr; ++col) {
            const double x = ctx.margin_left + col * card_w;
            measure_and_render_card(ctx, iter, x, row_y,
                                    static_cast<double>(card_w), false);
            iter = next_child_named(iter, "card");
        }

        row_y   += max_h + 8;
        total_h += max_h + 8;
        idx     += row_count;
    }

    return total_h + PARA_SPACING;
}

}  // namespace ase::viewer::render::dsl
