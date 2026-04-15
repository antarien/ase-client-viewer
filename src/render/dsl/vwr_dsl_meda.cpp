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

#include <ase/imgcache/image_cache.hpp>
#include "../vwr_rndr_image_path.hpp"
#include "render/nerdfont_table.hpp"

namespace ase::viewer::render::dsl {

namespace {

// Recursively walk an AST subtree to find the first NODE_IMAGE node.
// Used by render_figure / render_gallery to extract image URLs out of
// the figure / gallery directive's mixed paragraph children.
const ase::markdown::Node* find_first_image(const ase::markdown::Node* node) {
    if (node == nullptr) return nullptr;
    if (node->type == ase::markdown::NODE_IMAGE) return node;
    for (const ase::markdown::Node* c = node->first_child; c != nullptr; c = c->next_sibling) {
        const ase::markdown::Node* found = find_first_image(c);
        if (found != nullptr) return found;
    }
    return nullptr;
}

// Walk all immediate descendants depth-first and collect every NODE_IMAGE.
// Bounded to a small fixed array (gallery cell count rarely > 32).
constexpr uint32_t kMaxGalleryImages = 64;
struct ImgList {
    const ase::markdown::Node* items[kMaxGalleryImages];
    uint32_t count = 0;
};

void collect_images(const ase::markdown::Node* node, ImgList& out) {
    if (node == nullptr || out.count >= kMaxGalleryImages) return;
    if (node->type == ase::markdown::NODE_IMAGE) {
        out.items[out.count++] = node;
        return;
    }
    for (const ase::markdown::Node* c = node->first_child; c != nullptr; c = c->next_sibling) {
        collect_images(c, out);
    }
}

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
    //   title bar: flex gap-2, py-2 px-4 (8/16), 3×10 px window dots
    //              (red/yellow/green), title text-xs TEXT_PANEL_MUTED
    //   body: px-4 py-3 (16/12), text-sm (14), font-mono,
    //         leading-relaxed (1.625), TEXT_TERMINAL, prompt $ → PANEL_GREEN
    //
    // The old line-by-line path used a fixed LINE_H of 18 px which does
    // not match Pango's natural line height for text-sm + wrap, so long
    // lines that wrapped collapsed visually (next logical line drawn on
    // top of the wrap). Single-layout path lets Pango size correctly,
    // uses get_pixel_size for the actual drawn height, and keeps the
    // green-prompt highlighting via a span-merged markup string.
    constexpr double HDR_PX     = 16.0;  // px-4
    constexpr double HDR_PY     = 8.0;   // py-2
    constexpr double BODY_PX    = 16.0;  // px-4
    constexpr double BODY_PY    = 12.0;  // py-3
    constexpr int    TXT_PX     = 14;    // text-sm
    constexpr int    TITLE_PX   = 12;    // text-xs
    constexpr double DOT_D      = 10.0;  // 10 px window dots
    constexpr double DOT_GAP    = 6.0;   // gap-1.5
    constexpr double RADIUS     = 6.0;
    constexpr double HEADER_H   = DOT_D + 2 * HDR_PY;

    std::string text;
    ::ase::viewer::render::collect_plain_text(node, text);

    const char* title_attr = get_attr(node, "title");

    // Header bar background.
    ctx.cr->set_source_rgb(rgb_r(::ase::colors::CMS_TERMINAL_HEADER_BG),
                           rgb_g(::ase::colors::CMS_TERMINAL_HEADER_BG),
                           rgb_b(::ase::colors::CMS_TERMINAL_HEADER_BG));
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y, cw, HEADER_H, RADIUS);
    ctx.cr->fill();

    // Window dots (red, yellow, green) — web 1:1.
    double dx = ctx.margin_left + HDR_PX;
    const double dy = ctx.y + HDR_PY + DOT_D / 2.0;
    for (int i = 0; i < 3; ++i) {
        const uint32_t argb = (i == 0) ? ::ase::colors::PANEL_RED
                           :  (i == 1) ? ::ase::colors::PANEL_YELLOW
                                       : ::ase::colors::PANEL_GREEN;
        ctx.cr->set_source_rgb(rgb_r(argb), rgb_g(argb), rgb_b(argb));
        ctx.cr->arc(dx + DOT_D / 2.0, dy, DOT_D / 2.0, 0, 2 * M_PI);
        ctx.cr->fill();
        dx += DOT_D + DOT_GAP;
    }
    dx += 6.0;  // ml-2 before title

    // Title label.
    const char* title_text = (title_attr != nullptr) ? title_attr : "Terminal";
    auto title_layout = create_body_layout(ctx.cr, cw);
    auto title_fd = title_layout->get_font_description();
    title_fd.set_size(TITLE_PX * Pango::SCALE);
    title_layout->set_font_description(title_fd);
    {
        std::string tm;
        ::ase::viewer::render::append_escaped(tm, title_text);
        title_layout->set_markup(tm);
    }
    int tlw = 0, tlh = 0;
    title_layout->get_pixel_size(tlw, tlh);
    ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_PANEL_MUTED),
                           rgb_g(::ase::colors::TEXT_PANEL_MUTED),
                           rgb_b(::ase::colors::TEXT_PANEL_MUTED));
    draw_layout(ctx.cr, title_layout, dx, ctx.y + (HEADER_H - tlh) / 2.0);

    // Body as a SINGLE Pango layout — green span wraps every `$ ` prompt
    // prefix. Any line break becomes literal `\n` in the markup since
    // Pango layouts honour embedded newlines and wrap long lines via
    // WORD_CHAR when they exceed the layout width.
    std::string body_markup;
    {
        bool at_line_start = true;
        for (uint32_t i = 0; i < text.size(); ++i) {
            char c = text[i];
            if (at_line_start && c == '$') {
                body_markup.append("<span foreground=\"");
                body_markup.append(color_hex(::ase::colors::PANEL_GREEN));
                body_markup.append("\">$</span>");
                at_line_start = false;
                continue;
            }
            if (c == '\n') {
                body_markup.push_back('\n');
                at_line_start = true;
                continue;
            }
            if (c == '<') { body_markup.append("&lt;");  at_line_start = false; continue; }
            if (c == '>') { body_markup.append("&gt;");  at_line_start = false; continue; }
            if (c == '&') { body_markup.append("&amp;"); at_line_start = false; continue; }
            body_markup.push_back(c);
            at_line_start = false;
        }
    }

    auto body_layout = create_code_layout(ctx.cr, cw - 2 * static_cast<int>(BODY_PX));
    auto body_fd = body_layout->get_font_description();
    body_fd.set_size(TXT_PX * Pango::SCALE);
    body_layout->set_font_description(body_fd);
    body_layout->set_markup(body_markup);
    int bw = 0, bh = 0;
    body_layout->get_pixel_size(bw, bh);

    const double body_h = static_cast<double>(bh) + 2 * BODY_PY;

    // Body background.
    ctx.cr->set_source_rgb(rgb_r(::ase::colors::CMS_TERMINAL_BG),
                           rgb_g(::ase::colors::CMS_TERMINAL_BG),
                           rgb_b(::ase::colors::CMS_TERMINAL_BG));
    ctx.cr->rectangle(ctx.margin_left, ctx.y + HEADER_H, cw, body_h);
    ctx.cr->fill();

    ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_TERMINAL),
                           rgb_g(::ase::colors::TEXT_TERMINAL),
                           rgb_b(::ase::colors::TEXT_TERMINAL));
    draw_layout(ctx.cr, body_layout,
                ctx.margin_left + BODY_PX, ctx.y + HEADER_H + BODY_PY);

    // Container border.
    ctx.cr->set_source_rgb(rgb_r(::ase::colors::CMS_TERMINAL_HEADER_BG),
                           rgb_g(::ase::colors::CMS_TERMINAL_HEADER_BG),
                           rgb_b(::ase::colors::CMS_TERMINAL_HEADER_BG));
    ctx.cr->set_line_width(1);
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y, cw, HEADER_H + body_h, RADIUS);
    ctx.cr->stroke();

    return HEADER_H + body_h + 16.0;  // my-4 outer
}

// Parse a comma-separated column list into a bounded array of (ptr,len)
// pointers into the original attr value. The source string is part of
// the ase::markdown doc arena so the pointers stay valid through the
// whole render pass. Up to 16 columns; anything beyond is silently
// truncated to keep the layout stable.
constexpr uint32_t kMaxMatrixCols = 16;

struct MatrixColSpec {
    const char* name = nullptr;
    uint32_t    len  = 0;
};

struct MatrixColList {
    MatrixColSpec items[kMaxMatrixCols];
    uint32_t      count = 0;
};

void parse_matrix_cols(const char* csv, MatrixColList& out) {
    if (csv == nullptr) return;
    uint32_t i = 0;
    while (csv[i] != '\0' && out.count < kMaxMatrixCols) {
        while (csv[i] == ' ') ++i;
        const uint32_t start = i;
        while (csv[i] != '\0' && csv[i] != ',') ++i;
        uint32_t end = i;
        while (end > start && csv[end - 1] == ' ') --end;
        if (end > start) {
            auto& s = out.items[out.count++];
            s.name = csv + start;
            s.len  = end - start;
        }
        if (csv[i] == ',') ++i;
    }
}

// Case-insensitive attribute lookup by (name, len) — the row's attrs use
// the lowercase column name (e.g. cols="Modul" → attr modul="…").
const char* row_cell_value(const ase::markdown::Node* row,
                           const char* key, uint32_t key_len) {
    if (row == nullptr || row->attrs == nullptr) return nullptr;
    for (uint16_t i = 0; i < row->attr_count; ++i) {
        const char* k = row->attrs[i].key;
        if (k == nullptr) continue;
        uint32_t ki = 0;
        while (k[ki] != '\0' && ki < key_len) {
            const char a = k[ki] >= 'A' && k[ki] <= 'Z' ? k[ki] + ('a' - 'A') : k[ki];
            const char b = key[ki] >= 'A' && key[ki] <= 'Z' ? key[ki] + ('a' - 'A') : key[ki];
            if (a != b) break;
            ++ki;
        }
        if (ki == key_len && k[ki] == '\0') return row->attrs[i].value;
    }
    return nullptr;
}

double render_matrix(RenderContext& ctx, const ase::markdown::Node* node, int cw) {
    // Web 1:1 (cmsMatrixBlock.tsx):
    //   container: my-4, border 1 px CMS_MATRIX_HEADER_BG, radius 4
    //   header cells: bg CMS_MATRIX_HEADER_BG, py-3 px-4 (12/16),
    //                 text-xs bold CMS_MATRIX_ACCENT
    //   body cells:   bg CMS_MATRIX_BG, py-2 px-4 (8/16), text-sm
    //                 (odd-row alt bg SURFACE_1_ALT),
    //                 bottom border 1 px CMS_MATRIX_HEADER_BG
    //
    // Rows are `::row{col_a="…" col_b="…"}` leaf directives — no body
    // content. Each cell's value is looked up by column name (case
    // insensitive) from the row's attribute map. Values starting with
    // `nf-` render as NerdFont icons in CMS_MATRIX_ACCENT.
    constexpr double HDR_PX      = 16.0;  // px-4
    constexpr double HDR_PY      = 12.0;  // py-3
    constexpr double BODY_PX     = 16.0;  // px-4
    constexpr double BODY_PY     = 8.0;   // py-2
    constexpr int    HDR_TXT_PX  = 12;    // text-xs
    constexpr int    BODY_TXT_PX = 14;    // text-sm
    constexpr double RADIUS      = 4.0;

    MatrixColList cols;
    parse_matrix_cols(get_attr(node, "cols"), cols);

    const uint32_t row_total = count_children_named(node, "row");
    if (cols.count == 0 && row_total == 0) return 16.0;

    // Compute per-column x positions by spreading cw uniformly. Web uses
    // `w-full` table with auto cell sizing, but without knowing intrinsic
    // text widths we approximate with equal share — close enough for the
    // demo matrices and avoids a two-pass layout.
    const int col_count = static_cast<int>(cols.count == 0 ? 1 : cols.count);
    const int col_w = cw / col_count;

    // Header row.
    ctx.cr->set_source_rgb(rgb_r(::ase::colors::CMS_MATRIX_HEADER_BG),
                           rgb_g(::ase::colors::CMS_MATRIX_HEADER_BG),
                           rgb_b(::ase::colors::CMS_MATRIX_HEADER_BG));
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y, cw,
                 HDR_TXT_PX + 2 * HDR_PY, RADIUS);
    ctx.cr->fill();

    for (uint32_t c = 0; c < cols.count; ++c) {
        std::string h_markup;
        ::ase::viewer::render::append_escaped(h_markup, cols.items[c].name, cols.items[c].len);
        auto h_layout = create_body_layout(ctx.cr, col_w - 2 * static_cast<int>(HDR_PX));
        auto fd = h_layout->get_font_description();
        fd.set_weight(Pango::Weight::BOLD);
        fd.set_size(HDR_TXT_PX * Pango::SCALE);
        h_layout->set_font_description(fd);
        h_layout->set_markup(h_markup);
        ctx.cr->set_source_rgb(rgb_r(::ase::colors::CMS_MATRIX_ACCENT),
                               rgb_g(::ase::colors::CMS_MATRIX_ACCENT),
                               rgb_b(::ase::colors::CMS_MATRIX_ACCENT));
        draw_layout(ctx.cr, h_layout,
                    ctx.margin_left + c * col_w + HDR_PX, ctx.y + HDR_PY);
    }
    double total_h = HDR_TXT_PX + 2 * HDR_PY;

    // Body rows.
    int row_idx = 0;
    for (const ase::markdown::Node* row = first_child_named(node, "row"); row != nullptr;
         row = next_child_named(row, "row")) {
        const double ry = ctx.y + total_h;

        // Pre-measure the tallest cell in this row so wrapping cells
        // (e.g. long "Schicht" labels) don't overlap the next row.
        int max_cell_h = BODY_TXT_PX;
        Glib::RefPtr<Pango::Layout> cell_layouts[kMaxMatrixCols];
        for (uint32_t c = 0; c < cols.count; ++c) {
            const char* val = row_cell_value(row, cols.items[c].name, cols.items[c].len);
            std::string cm;
            if (val != nullptr) {
                if (val[0] == 'n' && val[1] == 'f' && val[2] == '-') {
                    const char* glyph = ::ase::viewer::render::lookup_nerdfont(val,
                        ::ase::viewer::render::cstr_len(val));
                    if (glyph != nullptr) {
                        cm.append("<span font_family=\"Symbols Nerd Font Mono\" foreground=\"");
                        cm.append(color_hex(::ase::colors::CMS_MATRIX_ACCENT));
                        cm.append("\">");
                        cm.append(glyph);
                        cm.append("</span>");
                    } else {
                        ::ase::viewer::render::append_escaped(cm, val);
                    }
                } else {
                    ::ase::viewer::render::append_escaped(cm, val);
                }
            }
            auto cell_layout = create_body_layout(ctx.cr, col_w - 2 * static_cast<int>(BODY_PX));
            auto cfd = cell_layout->get_font_description();
            cfd.set_size(BODY_TXT_PX * Pango::SCALE);
            cell_layout->set_font_description(cfd);
            cell_layout->set_markup(cm);
            int cw_ = 0, ch_ = 0;
            cell_layout->get_pixel_size(cw_, ch_);
            if (ch_ > max_cell_h) max_cell_h = ch_;
            cell_layouts[c] = cell_layout;
        }

        const double row_h = max_cell_h + 2 * BODY_PY;

        // Row background — alternate stripe.
        const uint32_t row_bg = (row_idx & 1)
            ? ::ase::colors::SURFACE_1_ALT
            : ::ase::colors::CMS_MATRIX_BG;
        ctx.cr->set_source_rgb(rgb_r(row_bg), rgb_g(row_bg), rgb_b(row_bg));
        ctx.cr->rectangle(ctx.margin_left, ry, cw, row_h);
        ctx.cr->fill();

        // Row bottom border.
        ctx.cr->set_source_rgb(rgb_r(::ase::colors::CMS_MATRIX_HEADER_BG),
                               rgb_g(::ase::colors::CMS_MATRIX_HEADER_BG),
                               rgb_b(::ase::colors::CMS_MATRIX_HEADER_BG));
        ctx.cr->set_line_width(1);
        ctx.cr->move_to(ctx.margin_left, ry + row_h);
        ctx.cr->line_to(ctx.margin_left + cw, ry + row_h);
        ctx.cr->stroke();

        // Draw cells.
        ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_PANEL_WHITE),
                               rgb_g(::ase::colors::TEXT_PANEL_WHITE),
                               rgb_b(::ase::colors::TEXT_PANEL_WHITE));
        for (uint32_t c = 0; c < cols.count; ++c) {
            if (!cell_layouts[c]) continue;
            draw_layout(ctx.cr, cell_layouts[c],
                        ctx.margin_left + c * col_w + BODY_PX, ry + BODY_PY);
        }

        total_h += row_h;
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

// ─────────────────────────────────────────────────────────────────────
// figure
// ─────────────────────────────────────────────────────────────────────
//
// Parse a CSS width value like "35%", "60%", "480px" into a pixel width
// bounded by the container width. Returns the container width as a
// fallback when the value is missing or malformed. Percent values round
// to the nearest pixel; "px" suffix is accepted and stripped. Any other
// unit (em/rem/vh/…) falls back to the container width since the native
// viewer has no CSS unit context.
int parse_css_width(const char* value, int container_w) {
    if (value == nullptr || value[0] == '\0') return container_w;
    uint32_t i = 0;
    double num = 0.0;
    bool has_digit = false;
    while (value[i] >= '0' && value[i] <= '9') {
        num = num * 10.0 + (value[i] - '0');
        ++i;
        has_digit = true;
    }
    if (value[i] == '.') {
        ++i;
        double frac = 0.1;
        while (value[i] >= '0' && value[i] <= '9') {
            num += (value[i] - '0') * frac;
            frac *= 0.1;
            ++i;
        }
    }
    if (!has_digit) return container_w;
    if (value[i] == '%') {
        int px = static_cast<int>(num * container_w / 100.0 + 0.5);
        if (px < 16) px = 16;
        if (px > container_w) px = container_w;
        return px;
    }
    // Accept bare number or "NNpx" as raw pixels.
    int px = static_cast<int>(num + 0.5);
    if (px < 16) px = 16;
    if (px > container_w) px = container_w;
    return px;
}

double render_figure(RenderContext& ctx, const ase::markdown::Node* node, int cw) {
    // Web 1:1 (cmsFigureBlock.tsx):
    //   container: my-4, border 1 BORDER_A20, radius 4
    //   floatStyle: align=left/right → float + horizontal margin 24
    //               align=center     → block centered
    //   width:      defaults to 100% for center, 40% for left/right
    //   image: width:100% inside figure
    //   caption: border-top BORDER_A20, padding 8/12, text-xs italic mono,
    //            color TEXT_LABEL, "Abbildung:" prefix (de) / "Figure:" (en)
    constexpr double RADIUS    = 4.0;
    constexpr double CAP_PAD_X = 12.0;
    constexpr double CAP_PAD_Y = 8.0;
    constexpr int    CAP_PX    = 12;

    const ase::markdown::Node* img = find_first_image(node);
    if (img == nullptr || img->url == nullptr) {
        return 16.0;
    }

    const std::string path = ::ase::viewer::render::resolve_image_path(img->url);

    const char* align_attr = get_attr(node, "align");
    const char* width_attr = get_attr(node, "width");

    const bool is_left   = (align_attr != nullptr &&
                            ::ase::viewer::render::streq(align_attr, "left"));
    const bool is_right  = (align_attr != nullptr &&
                            ::ase::viewer::render::streq(align_attr, "right"));
    const bool is_center = !is_left && !is_right;  // default

    int figure_w;
    if (width_attr != nullptr) {
        figure_w = parse_css_width(width_attr, cw);
    } else {
        figure_w = is_center ? cw : (cw * 40 / 100);
    }
    if (figure_w > cw) figure_w = cw;

    double figure_x;
    if (is_left)       figure_x = ctx.margin_left;
    else if (is_right) figure_x = ctx.margin_left + (cw - figure_w);
    else               figure_x = ctx.margin_left + (cw - figure_w) / 2.0;

    // Caption — use build_inline_content which correctly walks through
    // the figure's paragraph children and emits inline markup for EM,
    // STRONG, TEXT etc. while NODE_IMAGE falls through to an empty
    // build_inline_children call (no alt-text emission). No ad-hoc
    // subtree skipping needed.
    std::string caption_md = build_inline_content(node);

    // Pre-cache the image to know draw height at the figure width.
    const int img_max_w = figure_w - 2;
    auto entry = ::ase::imgcache::get(path, img_max_w);
    const int img_h = (entry.surface) ? entry.draw_height : 40;

    // Caption layout (built only if we have content).
    Glib::RefPtr<Pango::Layout> cap_layout;
    int cap_h = 0;
    if (!caption_md.empty()) {
        std::string prefixed = "<i>Abbildung: ";
        prefixed += caption_md;
        prefixed += "</i>";
        cap_layout = create_body_layout(ctx.cr, figure_w - 2 * static_cast<int>(CAP_PAD_X) - 2);
        auto fd = cap_layout->get_font_description();
        fd.set_size(CAP_PX * Pango::SCALE);
        cap_layout->set_font_description(fd);
        cap_layout->set_markup(prefixed);
        int unused = 0;
        cap_layout->get_pixel_size(unused, cap_h);
        cap_h += static_cast<int>(2 * CAP_PAD_Y);
    }

    const double total_h = img_h + cap_h + 2;

    // Image (1 px inset for border).
    ::ase::imgcache::render(ctx.cr, path,
                            figure_x + 1, ctx.y + 1, img_max_w);

    // Caption with top border.
    if (cap_h > 0 && cap_layout) {
        const double cap_y = ctx.y + img_h + 1;
        ctx.cr->set_source_rgba(rgb_r(::ase::colors::BORDER_A20),
                                rgb_g(::ase::colors::BORDER_A20),
                                rgb_b(::ase::colors::BORDER_A20),
                                rgb_a(::ase::colors::BORDER_A20));
        ctx.cr->set_line_width(1);
        ctx.cr->move_to(figure_x + 1, cap_y);
        ctx.cr->line_to(figure_x + figure_w - 1, cap_y);
        ctx.cr->stroke();

        ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_LABEL),
                               rgb_g(::ase::colors::TEXT_LABEL),
                               rgb_b(::ase::colors::TEXT_LABEL));
        int lw = 0, lh = 0;
        cap_layout->get_pixel_size(lw, lh);
        const double cx = figure_x + (figure_w - lw) / 2.0;
        draw_layout(ctx.cr, cap_layout, cx, cap_y + CAP_PAD_Y);
    }

    // Container outline (1 px BORDER_A20).
    ctx.cr->set_source_rgba(rgb_r(::ase::colors::BORDER_A20),
                            rgb_g(::ase::colors::BORDER_A20),
                            rgb_b(::ase::colors::BORDER_A20),
                            rgb_a(::ase::colors::BORDER_A20));
    ctx.cr->set_line_width(1);
    rounded_rect(ctx.cr, figure_x, ctx.y, figure_w, total_h, RADIUS);
    ctx.cr->stroke();

    return total_h + 16.0;  // my-4 outer
}

// ─────────────────────────────────────────────────────────────────────
// gallery
// ─────────────────────────────────────────────────────────────────────
double render_gallery(RenderContext& ctx, const ase::markdown::Node* node, int cw) {
    // Web 1:1 (cmsGalleryBlock.tsx):
    //   container: my-4, grid gap 0.5rem (8 px)
    //   cell: bg CMS_GALLERY_BG, border 1 BORDER_A20, radius 4,
    //         aspect-ratio 16/10
    constexpr double GAP    = 8.0;
    constexpr double RADIUS = 4.0;

    int cols = get_attr_int(node, "cols", 3);
    if (cols < 1) cols = 1;

    ImgList list;
    collect_images(node, list);
    if (list.count == 0) {
        return 16.0;
    }

    const int cell_w = (cw - static_cast<int>(GAP) * (cols - 1)) / cols;
    const int cell_h = (cell_w * 10) / 16;  // 16:10 aspect ratio

    int row = 0, col = 0;
    for (uint32_t i = 0; i < list.count; ++i) {
        const ase::markdown::Node* img = list.items[i];
        if (img->url == nullptr) continue;

        const double cell_x = ctx.margin_left + col * (cell_w + GAP);
        const double cell_y = ctx.y + row * (cell_h + GAP);

        // Cell background.
        ctx.cr->set_source_rgb(rgb_r(::ase::colors::CMS_GALLERY_BG),
                               rgb_g(::ase::colors::CMS_GALLERY_BG),
                               rgb_b(::ase::colors::CMS_GALLERY_BG));
        rounded_rect(ctx.cr, cell_x, cell_y, cell_w, cell_h, RADIUS);
        ctx.cr->fill();

        // Image (full cell, may be cropped if aspect mismatch).
        const std::string path = ::ase::viewer::render::resolve_image_path(img->url);
        ::ase::imgcache::render(ctx.cr, path, cell_x + 1, cell_y + 1, cell_w - 2);

        // Cell border.
        ctx.cr->set_source_rgba(rgb_r(::ase::colors::BORDER_A20),
                                rgb_g(::ase::colors::BORDER_A20),
                                rgb_b(::ase::colors::BORDER_A20),
                                rgb_a(::ase::colors::BORDER_A20));
        ctx.cr->set_line_width(1);
        rounded_rect(ctx.cr, cell_x, cell_y, cell_w, cell_h, RADIUS);
        ctx.cr->stroke();

        ++col;
        if (col >= cols) {
            col = 0;
            ++row;
        }
    }

    const int rows = (col == 0) ? row : (row + 1);
    const double total_h = rows * cell_h + (rows - 1) * GAP;
    return total_h + 16.0;  // my-4 outer
}

// ─────────────────────────────────────────────────────────────────────
// video
// ─────────────────────────────────────────────────────────────────────
double render_video(RenderContext& ctx, const ase::markdown::Node* node, int cw) {
    // Web 1:1 (cmsVideoLeaf.tsx):
    //   container: my-4, border 1 BORDER_A20, radius 4
    //   title bar (if title): bg CMS_VIDEO_HEADER_BG, py-2 px-3, text-xs
    //                         text TEXT_PANEL_WHITE, font-mono
    //   body: bg CMS_VIDEO_BG, 16:9 aspect
    //
    // Native viewer cannot embed iframes or play <video>; render the
    // body as a labeled placeholder showing icon + url.
    constexpr double RADIUS  = 4.0;
    constexpr double HDR_PX  = 12.0;  // px-3
    constexpr double HDR_PY  = 8.0;   // py-2
    constexpr int    TXT_PX  = 12;

    const char* url   = get_attr(node, "url");
    const char* title = get_attr(node, "title");

    const double hdr_h = (title != nullptr) ? (TXT_PX + 2 * HDR_PY) : 0.0;
    const double body_h = (cw * 9.0) / 16.0;  // 16:9
    const double total_h = hdr_h + body_h;

    // Header
    if (title != nullptr) {
        ctx.cr->set_source_rgb(rgb_r(::ase::colors::CMS_VIDEO_HEADER_BG),
                               rgb_g(::ase::colors::CMS_VIDEO_HEADER_BG),
                               rgb_b(::ase::colors::CMS_VIDEO_HEADER_BG));
        rounded_rect(ctx.cr, ctx.margin_left, ctx.y, cw, hdr_h, RADIUS);
        ctx.cr->fill();

        std::string title_markup;
        ::ase::viewer::render::append_escaped(title_markup, title);
        auto title_layout = create_body_layout(ctx.cr, cw - 2 * static_cast<int>(HDR_PX));
        auto fd = title_layout->get_font_description();
        fd.set_size(TXT_PX * Pango::SCALE);
        fd.set_weight(Pango::Weight::BOLD);
        title_layout->set_font_description(fd);
        title_layout->set_markup(title_markup);
        ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_PANEL_WHITE),
                               rgb_g(::ase::colors::TEXT_PANEL_WHITE),
                               rgb_b(::ase::colors::TEXT_PANEL_WHITE));
        draw_layout(ctx.cr, title_layout, ctx.margin_left + HDR_PX, ctx.y + HDR_PY);
    }

    // Body
    ctx.cr->set_source_rgb(rgb_r(::ase::colors::CMS_VIDEO_BG),
                           rgb_g(::ase::colors::CMS_VIDEO_BG),
                           rgb_b(::ase::colors::CMS_VIDEO_BG));
    ctx.cr->rectangle(ctx.margin_left, ctx.y + hdr_h, cw, body_h);
    ctx.cr->fill();

    // Centered "▶ Video" label with URL underneath.
    if (url != nullptr) {
        std::string label_markup = "<span font_family=\"Symbols Nerd Font Mono\" foreground=\"";
        label_markup.append(color_hex(::ase::colors::PANEL_PURPLE));
        label_markup.append("\">\xee\xa6\x80</span>  <b>Video</b>");  // nf-fa-play-circle
        auto label_layout = create_body_layout(ctx.cr, cw);
        auto fd = label_layout->get_font_description();
        fd.set_size(16 * Pango::SCALE);
        label_layout->set_font_description(fd);
        label_layout->set_markup(label_markup);
        int lw = 0, lh = 0;
        label_layout->get_pixel_size(lw, lh);
        ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_PANEL_WHITE),
                               rgb_g(::ase::colors::TEXT_PANEL_WHITE),
                               rgb_b(::ase::colors::TEXT_PANEL_WHITE));
        draw_layout(ctx.cr, label_layout,
                    ctx.margin_left + (cw - lw) / 2.0,
                    ctx.y + hdr_h + (body_h - lh) / 2.0 - 10);

        std::string url_markup;
        ::ase::viewer::render::append_escaped(url_markup, url);
        auto url_layout = create_body_layout(ctx.cr, cw - 32);
        auto ufd = url_layout->get_font_description();
        ufd.set_size(10 * Pango::SCALE);
        url_layout->set_font_description(ufd);
        url_layout->set_markup(url_markup);
        int uw = 0, uh = 0;
        url_layout->get_pixel_size(uw, uh);
        ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_PANEL_MUTED),
                               rgb_g(::ase::colors::TEXT_PANEL_MUTED),
                               rgb_b(::ase::colors::TEXT_PANEL_MUTED));
        draw_layout(ctx.cr, url_layout,
                    ctx.margin_left + (cw - uw) / 2.0,
                    ctx.y + hdr_h + (body_h - lh) / 2.0 + 14);
    }

    // Container outline.
    ctx.cr->set_source_rgba(rgb_r(::ase::colors::BORDER_A20),
                            rgb_g(::ase::colors::BORDER_A20),
                            rgb_b(::ase::colors::BORDER_A20),
                            rgb_a(::ase::colors::BORDER_A20));
    ctx.cr->set_line_width(1);
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y, cw, total_h, RADIUS);
    ctx.cr->stroke();

    return total_h + 16.0;  // my-4 outer
}

// ─────────────────────────────────────────────────────────────────────
// audio
// ─────────────────────────────────────────────────────────────────────
double render_audio(RenderContext& ctx, const ase::markdown::Node* node, int cw) {
    // Web 1:1 (cmsAudioLeaf.tsx):
    //   container: my-4, bg CMS_AUDIO_BG, border 1 BORDER_A20, radius 4
    //   header: bg CMS_AUDIO_HEADER_BG, px-4 py-3 (16/12), gap-3
    //           PANEL_PURPLE music icon + title (sm bold WHITE) + artist (xs MUTED)
    //   body: 32 px audio player area (placeholder in native)
    constexpr double RADIUS  = 4.0;
    constexpr double HDR_PX  = 16.0;
    constexpr double HDR_PY  = 12.0;
    constexpr int    TITLE_PX  = 14;
    constexpr int    ARTIST_PX = 12;
    constexpr double BODY_H  = 56.0;  // px-4 py-3 + 32 player

    const char* title  = get_attr(node, "title");
    const char* artist = get_attr(node, "artist");

    const double hdr_h = (title || artist) ? (HDR_PY + TITLE_PX + 4 + ARTIST_PX + HDR_PY) : 0.0;
    const double total_h = hdr_h + BODY_H;

    // Header
    if (hdr_h > 0) {
        ctx.cr->set_source_rgb(rgb_r(::ase::colors::CMS_AUDIO_HEADER_BG),
                               rgb_g(::ase::colors::CMS_AUDIO_HEADER_BG),
                               rgb_b(::ase::colors::CMS_AUDIO_HEADER_BG));
        rounded_rect(ctx.cr, ctx.margin_left, ctx.y, cw, hdr_h, RADIUS);
        ctx.cr->fill();

        // Music icon (NerdFont)
        std::string icon_markup = "<span font_family=\"Symbols Nerd Font Mono\" foreground=\"";
        icon_markup.append(color_hex(::ase::colors::PANEL_PURPLE));
        icon_markup.append("\">\xef\x80\x81</span>");  // nf-fa-music
        auto icon_layout = create_body_layout(ctx.cr, 32);
        auto ifd = icon_layout->get_font_description();
        ifd.set_size(20 * Pango::SCALE);
        icon_layout->set_font_description(ifd);
        icon_layout->set_markup(icon_markup);
        draw_layout(ctx.cr, icon_layout, ctx.margin_left + HDR_PX, ctx.y + HDR_PY + 2);

        const double text_x = ctx.margin_left + HDR_PX + 32;
        double text_y = ctx.y + HDR_PY;

        if (title != nullptr) {
            std::string t_markup;
            ::ase::viewer::render::append_escaped(t_markup, title);
            auto t_layout = create_body_layout(ctx.cr, cw - static_cast<int>(text_x - ctx.margin_left) - static_cast<int>(HDR_PX));
            auto fd = t_layout->get_font_description();
            fd.set_size(TITLE_PX * Pango::SCALE);
            fd.set_weight(Pango::Weight::BOLD);
            t_layout->set_font_description(fd);
            t_layout->set_markup(t_markup);
            ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_PANEL_WHITE),
                                   rgb_g(::ase::colors::TEXT_PANEL_WHITE),
                                   rgb_b(::ase::colors::TEXT_PANEL_WHITE));
            draw_layout(ctx.cr, t_layout, text_x, text_y);
            text_y += TITLE_PX + 4;
        }
        if (artist != nullptr) {
            std::string a_markup;
            ::ase::viewer::render::append_escaped(a_markup, artist);
            auto a_layout = create_body_layout(ctx.cr, cw - static_cast<int>(text_x - ctx.margin_left) - static_cast<int>(HDR_PX));
            auto afd = a_layout->get_font_description();
            afd.set_size(ARTIST_PX * Pango::SCALE);
            a_layout->set_font_description(afd);
            a_layout->set_markup(a_markup);
            ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_PANEL_MUTED),
                                   rgb_g(::ase::colors::TEXT_PANEL_MUTED),
                                   rgb_b(::ase::colors::TEXT_PANEL_MUTED));
            draw_layout(ctx.cr, a_layout, text_x, text_y);
        }
    }

    // Body — placeholder audio player area
    ctx.cr->set_source_rgb(rgb_r(::ase::colors::CMS_AUDIO_BG),
                           rgb_g(::ase::colors::CMS_AUDIO_BG),
                           rgb_b(::ase::colors::CMS_AUDIO_BG));
    ctx.cr->rectangle(ctx.margin_left, ctx.y + hdr_h, cw, BODY_H);
    ctx.cr->fill();

    // Faux progress bar centered in body
    const double bar_w = cw - 2 * HDR_PX;
    const double bar_x = ctx.margin_left + HDR_PX;
    const double bar_y = ctx.y + hdr_h + (BODY_H - 4) / 2.0;
    ctx.cr->set_source_rgba(rgb_r(::ase::colors::BORDER_A30),
                            rgb_g(::ase::colors::BORDER_A30),
                            rgb_b(::ase::colors::BORDER_A30),
                            rgb_a(::ase::colors::BORDER_A30));
    rounded_rect(ctx.cr, bar_x, bar_y, bar_w, 4, 2);
    ctx.cr->fill();
    ctx.cr->set_source_rgb(rgb_r(::ase::colors::PANEL_PURPLE),
                           rgb_g(::ase::colors::PANEL_PURPLE),
                           rgb_b(::ase::colors::PANEL_PURPLE));
    rounded_rect(ctx.cr, bar_x, bar_y, bar_w * 0.35, 4, 2);
    ctx.cr->fill();

    // Container outline
    ctx.cr->set_source_rgba(rgb_r(::ase::colors::BORDER_A20),
                            rgb_g(::ase::colors::BORDER_A20),
                            rgb_b(::ase::colors::BORDER_A20),
                            rgb_a(::ase::colors::BORDER_A20));
    ctx.cr->set_line_width(1);
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y, cw, total_h, RADIUS);
    ctx.cr->stroke();

    return total_h + 16.0;  // my-4 outer
}

// ─────────────────────────────────────────────────────────────────────
// compare
// ─────────────────────────────────────────────────────────────────────
double render_compare(RenderContext& ctx, const ase::markdown::Node* node, int cw) {
    // Web 1:1 (cmsCompareBlock.tsx):
    //   container: my-4, flex gap-4 (16) stretch
    //   option: p-4 (16), bg CMS_COMPARE_BG, border 2 px (highlighted ?
    //           CMS_COMPARE_HIGHLIGHT : SURFACE_3), radius 4
    //   title:  text-sm bold mb-3 pb-2, border-bottom 1 px SURFACE_3,
    //           color (highlighted ? CMS_COMPARE_HIGHLIGHT : TEXT_PANEL_WHITE)
    //   body:   text-sm TEXT_PANEL_WHITE
    constexpr double GAP        = 16.0;
    constexpr double PAD        = 16.0;
    constexpr double RADIUS     = 4.0;
    constexpr double BORDER_W   = 2.0;
    constexpr int    TITLE_PX   = 14;
    constexpr int    BODY_PX    = 14;
    constexpr double TITLE_MB   = 12.0;  // mb-3
    constexpr double TITLE_PB   = 8.0;   // pb-2

    const uint32_t opt_count = count_children_named(node, "option");
    if (opt_count == 0) return 16.0;

    const int col_w = (cw - static_cast<int>(GAP) * (static_cast<int>(opt_count) - 1)) / static_cast<int>(opt_count);
    const int inner_w = col_w - 2 * static_cast<int>(PAD) - 2 * static_cast<int>(BORDER_W);

    // Pre-measure each option's content height to size the row uniformly.
    double max_content_h = 0.0;
    for (const ase::markdown::Node* opt = first_child_named(node, "option"); opt != nullptr;
         opt = next_child_named(opt, "option")) {
        const char* title_attr = get_attr(opt, "title");
        std::string body_md = build_inline_content(opt);
        auto body_layout = create_body_layout(ctx.cr, inner_w);
        auto fd = body_layout->get_font_description();
        fd.set_size(BODY_PX * Pango::SCALE);
        body_layout->set_font_description(fd);
        body_layout->set_markup(body_md);
        int bw = 0, bh = 0;
        body_layout->get_pixel_size(bw, bh);
        const double title_h = (title_attr != nullptr) ? (TITLE_PX + TITLE_MB + TITLE_PB) : 0.0;
        const double content_h = title_h + bh;
        if (content_h > max_content_h) max_content_h = content_h;
    }
    const double col_h = max_content_h + 2 * PAD;

    int idx = 0;
    for (const ase::markdown::Node* opt = first_child_named(node, "option"); opt != nullptr;
         opt = next_child_named(opt, "option")) {
        const char* title_attr = get_attr(opt, "title");
        const char* highlight_attr = get_attr(opt, "highlight");
        const bool highlighted = (highlight_attr != nullptr &&
                                  !::ase::viewer::render::streq(highlight_attr, "false"));

        const double col_x = ctx.margin_left + idx * (col_w + GAP);

        // Background
        ctx.cr->set_source_rgb(rgb_r(::ase::colors::CMS_COMPARE_BG),
                               rgb_g(::ase::colors::CMS_COMPARE_BG),
                               rgb_b(::ase::colors::CMS_COMPARE_BG));
        rounded_rect(ctx.cr, col_x, ctx.y, col_w, col_h, RADIUS);
        ctx.cr->fill();

        // Border (2 px)
        const uint32_t border_argb = highlighted
            ? ::ase::colors::CMS_COMPARE_HIGHLIGHT
            : ::ase::colors::SURFACE_3;
        ctx.cr->set_source_rgb(rgb_r(border_argb),
                               rgb_g(border_argb),
                               rgb_b(border_argb));
        ctx.cr->set_line_width(BORDER_W);
        rounded_rect(ctx.cr, col_x, ctx.y, col_w, col_h, RADIUS);
        ctx.cr->stroke();

        double cy = ctx.y + PAD;

        if (title_attr != nullptr) {
            std::string t_markup;
            ::ase::viewer::render::append_escaped(t_markup, title_attr);
            auto t_layout = create_body_layout(ctx.cr, inner_w);
            auto fd = t_layout->get_font_description();
            fd.set_size(TITLE_PX * Pango::SCALE);
            fd.set_weight(Pango::Weight::BOLD);
            t_layout->set_font_description(fd);
            t_layout->set_markup(t_markup);
            const uint32_t t_color = highlighted
                ? ::ase::colors::CMS_COMPARE_HIGHLIGHT
                : ::ase::colors::TEXT_PANEL_WHITE;
            ctx.cr->set_source_rgb(rgb_r(t_color), rgb_g(t_color), rgb_b(t_color));
            draw_layout(ctx.cr, t_layout, col_x + PAD, cy);
            cy += TITLE_PX + TITLE_PB;
            // Border-bottom under title
            ctx.cr->set_source_rgb(rgb_r(::ase::colors::SURFACE_3),
                                   rgb_g(::ase::colors::SURFACE_3),
                                   rgb_b(::ase::colors::SURFACE_3));
            ctx.cr->set_line_width(1);
            ctx.cr->move_to(col_x + PAD, cy);
            ctx.cr->line_to(col_x + col_w - PAD, cy);
            ctx.cr->stroke();
            cy += TITLE_MB - TITLE_PB;
        }

        // Body
        std::string body_md = build_inline_content(opt);
        auto body_layout = create_body_layout(ctx.cr, inner_w);
        auto bfd = body_layout->get_font_description();
        bfd.set_size(BODY_PX * Pango::SCALE);
        body_layout->set_font_description(bfd);
        body_layout->set_markup(body_md);
        ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_PANEL_WHITE),
                               rgb_g(::ase::colors::TEXT_PANEL_WHITE),
                               rgb_b(::ase::colors::TEXT_PANEL_WHITE));
        draw_layout(ctx.cr, body_layout, col_x + PAD, cy);

        ++idx;
    }

    return col_h + 16.0;  // my-4 outer
}

}  // namespace ase::viewer::render::dsl
