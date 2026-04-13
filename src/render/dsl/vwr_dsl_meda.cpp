/**
 * DSL "media" group renderers — :::callout, :::quote, :::aside, :::terminal, :::matrix.
 *
 * All five share the same Pango-layout strategy: walk inline children
 * with the shared markup builder, measure with a Pango layout, draw a
 * tinted rounded background plus a left accent bar, then render the
 * content through draw_layout(). No Cairo Toy text API, no
 * std::stringstream, no std::vector — std-library terminal line
 * splitting is hand-written.
 *
 * @module      ase-client-viewer
 * @layer       5 (Clients)
 */

#include "vwr_dsl_meda.hpp"

namespace ase::viewer::render::dsl {

namespace {

struct CalloutStyle { double r, g, b; };

CalloutStyle callout_style_for(const char* type_attr) {
    if (type_attr == nullptr) return {INFO_R, INFO_G, INFO_B};
    if (::ase::viewer::render::streq(type_attr, "warning") ||
        ::ase::viewer::render::streq(type_attr, "warn"))    return {AMBER_R, AMBER_G, AMBER_B};
    if (::ase::viewer::render::streq(type_attr, "tip") ||
        ::ase::viewer::render::streq(type_attr, "success")) return {GREEN_R, GREEN_G, GREEN_B};
    if (::ase::viewer::render::streq(type_attr, "note"))    return {NOTE_R,  NOTE_G,  NOTE_B};
    if (::ase::viewer::render::streq(type_attr, "danger") ||
        ::ase::viewer::render::streq(type_attr, "error"))   return {0.94, 0.32, 0.29};
    return {INFO_R, INFO_G, INFO_B};
}

}  // namespace

double render_callout(RenderContext& ctx, const ase::markdown::Node* node, int cw) {
    const CalloutStyle s = callout_style_for(get_attr(node, "type"));
    const char* title_attr = get_attr(node, "title");

    std::string body_markup = build_inline_content(node);
    auto body_layout = create_body_layout(ctx.cr, cw - 40);
    body_layout->set_markup(body_markup);
    int bw = 0, bh = 0;
    body_layout->get_pixel_size(bw, bh);

    const double title_h = (title_attr != nullptr) ? 22.0 : 0.0;
    const double total_h = title_h + bh + 2 * BLOCK_PAD;

    ctx.cr->set_source_rgba(s.r, s.g, s.b, 0.06);
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y, cw, total_h, 4);
    ctx.cr->fill();

    ctx.cr->set_source_rgb(s.r, s.g, s.b);
    ctx.cr->rectangle(ctx.margin_left, ctx.y, 3, total_h);
    ctx.cr->fill();

    if (title_attr != nullptr) {
        std::string title_markup;
        ::ase::viewer::render::append_escaped(title_markup, title_attr);
        auto title_layout = create_body_layout(ctx.cr, cw - 40);
        auto fd = title_layout->get_font_description();
        fd.set_weight(Pango::Weight::BOLD);
        title_layout->set_font_description(fd);
        title_layout->set_markup(title_markup);
        ctx.cr->set_source_rgb(s.r, s.g, s.b);
        draw_layout(ctx.cr, title_layout, ctx.margin_left + 16, ctx.y + BLOCK_PAD);
    }

    ctx.cr->set_source_rgb(TEXT_R, TEXT_G, TEXT_B);
    draw_layout(ctx.cr, body_layout, ctx.margin_left + 16, ctx.y + BLOCK_PAD + title_h);

    return total_h + PARA_SPACING;
}

double render_aside(RenderContext& ctx, const ase::markdown::Node* node, int cw) {
    return render_callout(ctx, node, cw);
}

double render_quote(RenderContext& ctx, const ase::markdown::Node* node, int cw) {
    const char* author_attr = get_attr(node, "author");
    const char* role_attr   = get_attr(node, "role");

    std::string body_markup = build_inline_content(node);
    auto body_layout = create_body_layout(ctx.cr, cw - 40);
    auto fd = body_layout->get_font_description();
    fd.set_style(Pango::Style::ITALIC);
    body_layout->set_font_description(fd);
    body_layout->set_markup(body_markup);
    int bw = 0, bh = 0;
    body_layout->get_pixel_size(bw, bh);

    const double attr_h  = (author_attr != nullptr) ? 22.0 : 0.0;
    const double total_h = bh + 2 * BLOCK_PAD + attr_h;

    ctx.cr->set_source_rgba(RED_R, RED_G, RED_B, 0.06);
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y, cw, total_h, 4);
    ctx.cr->fill();

    ctx.cr->set_source_rgb(RED_R, RED_G, RED_B);
    ctx.cr->rectangle(ctx.margin_left, ctx.y, 3, total_h);
    ctx.cr->fill();

    ctx.cr->set_source_rgb(TEXT_R, TEXT_G, TEXT_B);
    draw_layout(ctx.cr, body_layout, ctx.margin_left + 20, ctx.y + BLOCK_PAD);

    if (author_attr != nullptr) {
        std::string attr_markup = "— ";
        ::ase::viewer::render::append_escaped(attr_markup, author_attr);
        if (role_attr != nullptr) {
            attr_markup.append(", <i>");
            ::ase::viewer::render::append_escaped(attr_markup, role_attr);
            attr_markup.append("</i>");
        }
        auto attr_layout = create_body_layout(ctx.cr, cw - 40);
        attr_layout->set_markup(attr_markup);
        ctx.cr->set_source_rgb(MUTED_R, MUTED_G, MUTED_B);
        draw_layout(ctx.cr, attr_layout, ctx.margin_left + 20, ctx.y + BLOCK_PAD + bh + 4);
    }

    return total_h + PARA_SPACING;
}

double render_terminal(RenderContext& ctx, const ase::markdown::Node* node, int cw) {
    constexpr double HEADER_H = 24.0;

    std::string text;
    ::ase::viewer::render::collect_plain_text(node, text);

    // Header bar.
    ctx.cr->set_source_rgb(0.055, 0.071, 0.078);
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y, cw, HEADER_H, 4);
    ctx.cr->fill();

    auto title_layout = create_body_layout(ctx.cr, cw);
    auto title_fd = title_layout->get_font_description();
    title_fd.set_size(9 * Pango::SCALE);
    title_layout->set_font_description(title_fd);
    title_layout->set_text("Terminal");
    ctx.cr->set_source_rgb(MUTED_R, MUTED_G, MUTED_B);
    draw_layout(ctx.cr, title_layout, ctx.margin_left + 12, ctx.y + 6);

    // Body — manual line-by-line rendering with $-prompt highlighting.
    constexpr double LINE_H = 16.0;
    int line_count = 1;
    for (uint32_t i = 0; i < text.size(); ++i) if (text[i] == '\n') ++line_count;
    const double body_h = line_count * LINE_H + 2 * BLOCK_PAD;

    ctx.cr->set_source_rgb(SURFACE_R, SURFACE_G, SURFACE_B);
    ctx.cr->rectangle(ctx.margin_left, ctx.y + HEADER_H, cw, body_h);
    ctx.cr->fill();

    double ly = ctx.y + HEADER_H + BLOCK_PAD;
    uint32_t i = 0;
    while (i < text.size()) {
        uint32_t line_start = i;
        while (i < text.size() && text[i] != '\n') ++i;
        std::string line = text.substr(line_start, i - line_start);
        if (i < text.size()) ++i;

        auto layout = create_code_layout(ctx.cr, cw - 24);
        std::string markup;
        if (!line.empty() && line[0] == '$') {
            markup.append("<span foreground=\"#30a848\">$</span> ");
            ::ase::viewer::render::append_escaped(markup, line.c_str() + 1,
                                                  static_cast<uint32_t>(line.size() - 1));
            layout->set_markup(markup);
            ctx.cr->set_source_rgb(TEXT_R, TEXT_G, TEXT_B);
        } else {
            ::ase::viewer::render::append_escaped(markup, line.c_str(),
                                                  static_cast<uint32_t>(line.size()));
            layout->set_markup(markup);
            ctx.cr->set_source_rgb(MUTED_R, MUTED_G, MUTED_B);
        }
        draw_layout(ctx.cr, layout, ctx.margin_left + 12, ly);
        ly += LINE_H;
    }

    return HEADER_H + body_h + PARA_SPACING;
}

double render_matrix(RenderContext& ctx, const ase::markdown::Node* node, int cw) {
    if (count_children_named(node, "row") == 0) return PARA_SPACING;

    constexpr double ROW_H = 28.0;
    constexpr double PAD   = 6.0;

    double total_h = 0.0;
    ctx.cr->set_source_rgb(0.055, 0.078, 0.055);
    ctx.cr->rectangle(ctx.margin_left, ctx.y, cw, ROW_H);
    ctx.cr->fill();

    const char* title_attr = get_attr(node, "title");
    if (title_attr != nullptr) {
        std::string title_markup;
        ::ase::viewer::render::append_escaped(title_markup, title_attr);
        auto layout = create_body_layout(ctx.cr, cw - 12);
        auto fd = layout->get_font_description();
        fd.set_weight(Pango::Weight::BOLD);
        fd.set_size(10 * Pango::SCALE);
        layout->set_font_description(fd);
        layout->set_markup(title_markup);
        ctx.cr->set_source_rgb(GREEN_R, GREEN_G, GREEN_B);
        draw_layout(ctx.cr, layout, ctx.margin_left + PAD, ctx.y + 6);
    }
    total_h += ROW_H;

    int row_idx = 0;
    for (const ase::markdown::Node* row = first_child_named(node, "row"); row != nullptr;
         row = next_child_named(row, "row")) {
        const double ry = ctx.y + total_h;
        if (row_idx % 2 == 0) {
            ctx.cr->set_source_rgba(1.0, 1.0, 1.0, 0.015);
            ctx.cr->rectangle(ctx.margin_left, ry, cw, ROW_H);
            ctx.cr->fill();
        }

        std::string markup = build_inline_content(row);
        auto layout = create_body_layout(ctx.cr, cw - 12);
        auto fd = layout->get_font_description();
        fd.set_size(10 * Pango::SCALE);
        layout->set_font_description(fd);
        layout->set_markup(markup);

        ctx.cr->set_source_rgb(TEXT_R, TEXT_G, TEXT_B);
        draw_layout(ctx.cr, layout, ctx.margin_left + PAD, ry + 6);

        total_h += ROW_H;
        ++row_idx;
    }

    return total_h + PARA_SPACING;
}

}  // namespace ase::viewer::render::dsl
