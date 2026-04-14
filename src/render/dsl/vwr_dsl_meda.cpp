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
    // Web 1:1 (cmsCustomCallout.tsx):
    //   container: my-6 mb-4 (24/16 outer), px-5 py-4 (20/16 inner)
    //   border-left: 3 px solid accent
    //   radius: 0 4 4 0 (right side only — approximated as 4 all sides
    //                    until rounded_rect supports per-corner radii)
    //   bg: DOCS_CALLOUT_*_BG semantic tint
    //   header (icon + title): gap-2 (8) + mb-2 (8)
    //   body: TEXT_PANEL_MUTED, monospace
    constexpr double PAD_X       = 20.0;  // px-5
    constexpr double PAD_Y       = 16.0;  // py-4
    constexpr double BORDER_W    = 3.0;
    constexpr double RADIUS      = 4.0;
    constexpr double TITLE_MB    = 8.0;   // mb-2
    constexpr int    TXT_PX      = 14;    // text-sm

    const CalloutStyle s = callout_style_for(get_attr(node, "type"));
    const char* title_attr = get_attr(node, "title");

    std::string body_markup = build_inline_content(node);
    auto body_layout = create_body_layout(ctx.cr, cw - 2 * static_cast<int>(PAD_X));
    auto body_fd = body_layout->get_font_description();
    body_fd.set_size(TXT_PX * Pango::SCALE);
    body_layout->set_font_description(body_fd);
    body_layout->set_markup(body_markup);
    int bw = 0, bh = 0;
    body_layout->get_pixel_size(bw, bh);

    const double title_h = (title_attr != nullptr) ? (TXT_PX + TITLE_MB) : 0.0;
    const double total_h = title_h + bh + 2 * PAD_Y;

    ctx.cr->set_source_rgba(s.r, s.g, s.b, 0.06);
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y, cw, total_h, RADIUS);
    ctx.cr->fill();

    ctx.cr->set_source_rgb(s.r, s.g, s.b);
    ctx.cr->rectangle(ctx.margin_left, ctx.y, BORDER_W, total_h);
    ctx.cr->fill();

    if (title_attr != nullptr) {
        std::string title_markup;
        ::ase::viewer::render::append_escaped(title_markup, title_attr);
        auto title_layout = create_body_layout(ctx.cr, cw - 2 * static_cast<int>(PAD_X));
        auto fd = title_layout->get_font_description();
        fd.set_weight(Pango::Weight::BOLD);
        fd.set_size(TXT_PX * Pango::SCALE);
        title_layout->set_font_description(fd);
        title_layout->set_markup(title_markup);
        ctx.cr->set_source_rgb(s.r, s.g, s.b);
        draw_layout(ctx.cr, title_layout, ctx.margin_left + PAD_X, ctx.y + PAD_Y);
    }

    ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_PANEL_MUTED),
                           rgb_g(::ase::colors::TEXT_PANEL_MUTED),
                           rgb_b(::ase::colors::TEXT_PANEL_MUTED));
    draw_layout(ctx.cr, body_layout, ctx.margin_left + PAD_X, ctx.y + PAD_Y + title_h);

    // my-6 mb-4 outer = 24 + 16.
    return total_h + 24.0 + 16.0;
}

double render_aside(RenderContext& ctx, const ase::markdown::Node* node, int cw) {
    return render_callout(ctx, node, cw);
}

double render_quote(RenderContext& ctx, const ase::markdown::Node* node, int cw) {
    // Web parity (callout-style red border-left, larger horizontal padding).
    constexpr double PAD_X    = 20.0;  // px-5
    constexpr double PAD_Y    = 16.0;  // py-4
    constexpr double BORDER_W = 3.0;
    constexpr double RADIUS   = 4.0;
    constexpr int    TXT_PX   = 14;    // text-sm

    const char* author_attr = get_attr(node, "author");
    const char* role_attr   = get_attr(node, "role");

    std::string body_markup = build_inline_content(node);
    auto body_layout = create_body_layout(ctx.cr, cw - 2 * static_cast<int>(PAD_X));
    auto fd = body_layout->get_font_description();
    fd.set_style(Pango::Style::ITALIC);
    fd.set_size(TXT_PX * Pango::SCALE);
    body_layout->set_font_description(fd);
    body_layout->set_markup(body_markup);
    int bw = 0, bh = 0;
    body_layout->get_pixel_size(bw, bh);

    const double attr_h  = (author_attr != nullptr) ? (TXT_PX + 8.0) : 0.0;
    const double total_h = bh + 2 * PAD_Y + attr_h;

    ctx.cr->set_source_rgba(RED_R, RED_G, RED_B, 0.06);
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y, cw, total_h, RADIUS);
    ctx.cr->fill();

    ctx.cr->set_source_rgb(RED_R, RED_G, RED_B);
    ctx.cr->rectangle(ctx.margin_left, ctx.y, BORDER_W, total_h);
    ctx.cr->fill();

    ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_PANEL_WHITE),
                           rgb_g(::ase::colors::TEXT_PANEL_WHITE),
                           rgb_b(::ase::colors::TEXT_PANEL_WHITE));
    draw_layout(ctx.cr, body_layout, ctx.margin_left + PAD_X, ctx.y + PAD_Y);

    if (author_attr != nullptr) {
        std::string attr_markup = "— ";
        ::ase::viewer::render::append_escaped(attr_markup, author_attr);
        if (role_attr != nullptr) {
            attr_markup.append(", <i>");
            ::ase::viewer::render::append_escaped(attr_markup, role_attr);
            attr_markup.append("</i>");
        }
        auto attr_layout = create_body_layout(ctx.cr, cw - 2 * static_cast<int>(PAD_X));
        auto attr_fd = attr_layout->get_font_description();
        attr_fd.set_size(TXT_PX * Pango::SCALE);
        attr_layout->set_font_description(attr_fd);
        attr_layout->set_markup(attr_markup);
        ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_PANEL_MUTED),
                               rgb_g(::ase::colors::TEXT_PANEL_MUTED),
                               rgb_b(::ase::colors::TEXT_PANEL_MUTED));
        draw_layout(ctx.cr, attr_layout, ctx.margin_left + PAD_X, ctx.y + PAD_Y + bh + 4);
    }

    return total_h + 16.0;
}

double render_terminal(RenderContext& ctx, const ase::markdown::Node* node, int cw) {
    // Web 1:1 (cmsTerminalBlock.tsx):
    //   container: my-4, border 1 px CMS_TERMINAL_HEADER_BG, radius 6
    //   title bar: bg CMS_TERMINAL_HEADER_BG, py-2 px-4 (8/16), gap-2
    //   body: bg CMS_TERMINAL_BG, py-3 px-4 (12/16), text-sm (14)
    //         text TEXT_TERMINAL, prompt $ → PANEL_GREEN
    constexpr double HDR_PX     = 16.0;  // px-4
    constexpr double HDR_PY     = 8.0;   // py-2
    constexpr double BODY_PX    = 16.0;  // px-4
    constexpr double BODY_PY    = 12.0;  // py-3
    constexpr int    TXT_PX     = 14;    // text-sm
    constexpr double LINE_H     = TXT_PX + 4.0;
    constexpr double RADIUS     = 6.0;
    constexpr double HEADER_H   = TXT_PX + 2 * HDR_PY;

    std::string text;
    ::ase::viewer::render::collect_plain_text(node, text);

    // Header bar.
    ctx.cr->set_source_rgb(rgb_r(::ase::colors::CMS_TERMINAL_HEADER_BG),
                           rgb_g(::ase::colors::CMS_TERMINAL_HEADER_BG),
                           rgb_b(::ase::colors::CMS_TERMINAL_HEADER_BG));
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y, cw, HEADER_H, RADIUS);
    ctx.cr->fill();

    auto title_layout = create_body_layout(ctx.cr, cw);
    auto title_fd = title_layout->get_font_description();
    title_fd.set_size(TXT_PX * Pango::SCALE);
    title_layout->set_font_description(title_fd);
    title_layout->set_text("Terminal");
    ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_PANEL_MUTED),
                           rgb_g(::ase::colors::TEXT_PANEL_MUTED),
                           rgb_b(::ase::colors::TEXT_PANEL_MUTED));
    draw_layout(ctx.cr, title_layout, ctx.margin_left + HDR_PX, ctx.y + HDR_PY);

    // Body — manual line-by-line rendering with $-prompt highlighting.
    int line_count = 1;
    for (uint32_t i = 0; i < text.size(); ++i) if (text[i] == '\n') ++line_count;
    const double body_h = line_count * LINE_H + 2 * BODY_PY;

    ctx.cr->set_source_rgb(rgb_r(::ase::colors::CMS_TERMINAL_BG),
                           rgb_g(::ase::colors::CMS_TERMINAL_BG),
                           rgb_b(::ase::colors::CMS_TERMINAL_BG));
    ctx.cr->rectangle(ctx.margin_left, ctx.y + HEADER_H, cw, body_h);
    ctx.cr->fill();

    double ly = ctx.y + HEADER_H + BODY_PY;
    uint32_t i = 0;
    while (i < text.size()) {
        uint32_t line_start = i;
        while (i < text.size() && text[i] != '\n') ++i;
        std::string line = text.substr(line_start, i - line_start);
        if (i < text.size()) ++i;

        auto layout = create_code_layout(ctx.cr, cw - 2 * static_cast<int>(BODY_PX));
        auto layout_fd = layout->get_font_description();
        layout_fd.set_size(TXT_PX * Pango::SCALE);
        layout->set_font_description(layout_fd);
        std::string markup;
        if (!line.empty() && line[0] == '$') {
            markup.append("<span foreground=\"");
            markup.append(color_hex(::ase::colors::PANEL_GREEN));
            markup.append("\">$</span> ");
            ::ase::viewer::render::append_escaped(markup, line.c_str() + 1,
                                                  static_cast<uint32_t>(line.size() - 1));
            layout->set_markup(markup);
            ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_TERMINAL),
                                   rgb_g(::ase::colors::TEXT_TERMINAL),
                                   rgb_b(::ase::colors::TEXT_TERMINAL));
        } else {
            ::ase::viewer::render::append_escaped(markup, line.c_str(),
                                                  static_cast<uint32_t>(line.size()));
            layout->set_markup(markup);
            ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_TERMINAL),
                                   rgb_g(::ase::colors::TEXT_TERMINAL),
                                   rgb_b(::ase::colors::TEXT_TERMINAL));
        }
        draw_layout(ctx.cr, layout, ctx.margin_left + BODY_PX, ly);
        ly += LINE_H;
    }

    // Container border.
    ctx.cr->set_source_rgb(rgb_r(::ase::colors::CMS_TERMINAL_HEADER_BG),
                           rgb_g(::ase::colors::CMS_TERMINAL_HEADER_BG),
                           rgb_b(::ase::colors::CMS_TERMINAL_HEADER_BG));
    ctx.cr->set_line_width(1);
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y, cw, HEADER_H + body_h, RADIUS);
    ctx.cr->stroke();

    return HEADER_H + body_h + 16.0;  // my-4 outer
}

double render_matrix(RenderContext& ctx, const ase::markdown::Node* node, int cw) {
    // Web 1:1 (cmsMatrixBlock.tsx):
    //   container: my-4, border 1 px CMS_MATRIX_HEADER_BG, radius 4
    //   header: bg CMS_MATRIX_HEADER_BG, py-3 px-4 (12/16), text-xs bold
    //   body: bg CMS_MATRIX_BG, py-2 px-4 (8/16), text-sm,
    //         row-bottom border 1 px CMS_MATRIX_HEADER_BG
    constexpr double HDR_PX     = 16.0;  // px-4
    constexpr double HDR_PY     = 12.0;  // py-3
    constexpr double BODY_PX    = 16.0;  // px-4
    constexpr double BODY_PY    = 8.0;   // py-2
    constexpr int    HDR_TXT_PX = 12;    // text-xs
    constexpr int    BODY_TXT_PX = 14;   // text-sm
    constexpr double HDR_H      = HDR_TXT_PX + 2 * HDR_PY;
    constexpr double BODY_H     = BODY_TXT_PX + 2 * BODY_PY;
    constexpr double RADIUS     = 4.0;

    if (count_children_named(node, "row") == 0) return 16.0;

    double total_h = 0.0;

    ctx.cr->set_source_rgb(rgb_r(::ase::colors::CMS_MATRIX_HEADER_BG),
                           rgb_g(::ase::colors::CMS_MATRIX_HEADER_BG),
                           rgb_b(::ase::colors::CMS_MATRIX_HEADER_BG));
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y, cw, HDR_H, RADIUS);
    ctx.cr->fill();

    const char* title_attr = get_attr(node, "title");
    if (title_attr != nullptr) {
        std::string title_markup;
        ::ase::viewer::render::append_escaped(title_markup, title_attr);
        auto layout = create_body_layout(ctx.cr, cw - 2 * static_cast<int>(HDR_PX));
        auto fd = layout->get_font_description();
        fd.set_weight(Pango::Weight::BOLD);
        fd.set_size(HDR_TXT_PX * Pango::SCALE);
        layout->set_font_description(fd);
        layout->set_markup(title_markup);
        ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_PANEL_WHITE),
                               rgb_g(::ase::colors::TEXT_PANEL_WHITE),
                               rgb_b(::ase::colors::TEXT_PANEL_WHITE));
        draw_layout(ctx.cr, layout, ctx.margin_left + HDR_PX, ctx.y + HDR_PY);
    }
    total_h += HDR_H;

    int row_idx = 0;
    for (const ase::markdown::Node* row = first_child_named(node, "row"); row != nullptr;
         row = next_child_named(row, "row")) {
        const double ry = ctx.y + total_h;

        ctx.cr->set_source_rgb(rgb_r(::ase::colors::CMS_MATRIX_BG),
                               rgb_g(::ase::colors::CMS_MATRIX_BG),
                               rgb_b(::ase::colors::CMS_MATRIX_BG));
        ctx.cr->rectangle(ctx.margin_left, ry, cw, BODY_H);
        ctx.cr->fill();

        // Row bottom border.
        ctx.cr->set_source_rgb(rgb_r(::ase::colors::CMS_MATRIX_HEADER_BG),
                               rgb_g(::ase::colors::CMS_MATRIX_HEADER_BG),
                               rgb_b(::ase::colors::CMS_MATRIX_HEADER_BG));
        ctx.cr->set_line_width(1);
        ctx.cr->move_to(ctx.margin_left, ry + BODY_H);
        ctx.cr->line_to(ctx.margin_left + cw, ry + BODY_H);
        ctx.cr->stroke();

        std::string markup = build_inline_content(row);
        auto layout = create_body_layout(ctx.cr, cw - 2 * static_cast<int>(BODY_PX));
        auto fd = layout->get_font_description();
        fd.set_size(BODY_TXT_PX * Pango::SCALE);
        layout->set_font_description(fd);
        layout->set_markup(markup);

        ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_PANEL_WHITE),
                               rgb_g(::ase::colors::TEXT_PANEL_WHITE),
                               rgb_b(::ase::colors::TEXT_PANEL_WHITE));
        draw_layout(ctx.cr, layout, ctx.margin_left + BODY_PX, ry + BODY_PY);

        total_h += BODY_H;
        ++row_idx;
    }

    // Container outline.
    ctx.cr->set_source_rgb(rgb_r(::ase::colors::CMS_MATRIX_HEADER_BG),
                           rgb_g(::ase::colors::CMS_MATRIX_HEADER_BG),
                           rgb_b(::ase::colors::CMS_MATRIX_HEADER_BG));
    ctx.cr->set_line_width(1);
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y, cw, total_h, RADIUS);
    ctx.cr->stroke();

    return total_h + 16.0;  // my-4 outer
}

}  // namespace ase::viewer::render::dsl
