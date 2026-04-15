/*
 * ==============================================================================
 * ASE VIEWER — Document renderer
 * ==============================================================================
 *
 * @file        vwr_rndr_doc.cpp
 * @brief       AST → Cairo dispatcher for the native markdown viewer
 * @description Walks the ase::markdown POD AST, emits Cairo/Pango draw calls
 *              for every node type, accumulates y position, returns total
 *              content height. Inline children inside paragraphs are rendered
 *              via a Pango markup builder which preserves emphasis, strong,
 *              inline code, links, NerdFont icons (font-family switch via the
 *              build-generated nerdfont_table.hpp) and inline math (passed
 *              through to ase::adp::microtex). Block-level math, mermaid diagrams,
 *              syntax-highlighted code blocks and DSL directives are routed
 *              to their dedicated specialist renderers.
 *
 * @module      ase-client-viewer
 * @layer       5 (Clients)
 * @category    process/computation/algorithm
 *
 * @created     2026-04-12
 * @modified    2026-04-13
 * @version     00.00.19.00019 [poc]
 *
 * ==============================================================================
 * CLIENT INFRASTRUCTURE COMPLIANCE
 * ==============================================================================
 * [ ] No raw new/delete (smart pointers or stack only)
 * [ ] No std::vector / std::map / std::set in source-level data structures
 * [ ] No switch/case dispatch — use if/else ladders
 * [ ] No std::stringstream / std::strcmp / std::strlen
 * [ ] All inline rich-text routed through Pango markup, never collect_text()
 * [ ] NerdFont icons rendered via build-generated nerdfont_table.hpp
 * [ ] Math handled by ase::adp::microtex adapter (adapter/ase-adp-microtex)
 * [ ] Mermaid diagrams routed to render_mermaid()
 * [ ] All block heights measured before drawing (no fixed magic numbers)
 * ==============================================================================
 */

#include "vwr_rndr_doc.hpp"
#include "vwr_rndr_text.hpp"
#include "vwr_rndr_inline.hpp"
#include "vwr_rndr_mmrd.hpp"
#include "vwr_rndr_math.hpp"
#include "vwr_rndr_amdl.hpp"
#include "vwr_rndr_image_path.hpp"
#include <ase/imgcache/image_cache.hpp>
#include "vwr_rndr_synt.hpp"
#include "dsl/vwr_dsl_dispatch.hpp"

#include "render/nerdfont_table.hpp"

#include <ase/markdown/types.hpp>

// Generated color SSOT (sha-web-console/generated/) — for BORDER_A30 etc.
#include "colors.hpp"

#include <ase/utils/strops.hpp>

#include <cairo.h>
#include <pango/pango.h>
#include <glib.h>

#include <string>

namespace ase::viewer::render {

namespace {

// ── Color SSOT — derived from ase::colors::* (generated from colors.ts)
//
// Channels are unpacked from each 0xAARRGGBB constant via inline shifts so
// the underlying hex stays in the generated header and never lives in two
// places. Never write hex literals or 0x?? / 255.0 in this file again —
// add the missing constant to colors.ts, regenerate, then reference it.
constexpr double rgb_r(uint32_t argb) { return static_cast<double>((argb >> 16) & 0xFFu) / 255.0; }
constexpr double rgb_g(uint32_t argb) { return static_cast<double>((argb >>  8) & 0xFFu) / 255.0; }
constexpr double rgb_b(uint32_t argb) { return static_cast<double>((argb      ) & 0xFFu) / 255.0; }
constexpr double rgb_a(uint32_t argb) { return static_cast<double>((argb >> 24) & 0xFFu) / 255.0; }

// Heading palette.
constexpr double H1_R = rgb_r(::ase::colors::DOCS_H1), H1_G = rgb_g(::ase::colors::DOCS_H1), H1_B = rgb_b(::ase::colors::DOCS_H1);
constexpr double H2_R = rgb_r(::ase::colors::DOCS_H2), H2_G = rgb_g(::ase::colors::DOCS_H2), H2_B = rgb_b(::ase::colors::DOCS_H2);
constexpr double H3_R = rgb_r(::ase::colors::DOCS_H3), H3_G = rgb_g(::ase::colors::DOCS_H3), H3_B = rgb_b(::ase::colors::DOCS_H3);
constexpr double H4_R = rgb_r(::ase::colors::DOCS_H4), H4_G = rgb_g(::ase::colors::DOCS_H4), H4_B = rgb_b(::ase::colors::DOCS_H4);

// Body / text palette.
constexpr double TEXT_R   = rgb_r(::ase::colors::TEXT_PRIMARY),     TEXT_G   = rgb_g(::ase::colors::TEXT_PRIMARY),     TEXT_B   = rgb_b(::ase::colors::TEXT_PRIMARY);
constexpr double DOCSFG_R = rgb_r(::ase::colors::TEXT_DOCS_FG),     DOCSFG_G = rgb_g(::ase::colors::TEXT_DOCS_FG),     DOCSFG_B = rgb_b(::ase::colors::TEXT_DOCS_FG);
constexpr double MUTED_R  = rgb_r(::ase::colors::TEXT_PANEL_MUTED), MUTED_G  = rgb_g(::ase::colors::TEXT_PANEL_MUTED), MUTED_B  = rgb_b(::ase::colors::TEXT_PANEL_MUTED);
constexpr double LINK_R   = rgb_r(::ase::colors::PANEL_CYAN),       LINK_G   = rgb_g(::ase::colors::PANEL_CYAN),       LINK_B   = rgb_b(::ase::colors::PANEL_CYAN);

// Code block + table surfaces.
constexpr double CODE_BG_R = rgb_r(::ase::colors::DOCS_CODE_BG), CODE_BG_G = rgb_g(::ase::colors::DOCS_CODE_BG), CODE_BG_B = rgb_b(::ase::colors::DOCS_CODE_BG);

constexpr double TBL_BG_R  = rgb_r(::ase::colors::DOCS_TABLE_BG),       TBL_BG_G  = rgb_g(::ase::colors::DOCS_TABLE_BG),       TBL_BG_B  = rgb_b(::ase::colors::DOCS_TABLE_BG);
constexpr double TBL_HD_R  = rgb_r(::ase::colors::DOCS_TABLE_HEADER),   TBL_HD_G  = rgb_g(::ase::colors::DOCS_TABLE_HEADER),   TBL_HD_B  = rgb_b(::ase::colors::DOCS_TABLE_HEADER);
constexpr double TBL_ALT_R = rgb_r(::ase::colors::DOCS_TABLE_ROW_ALT),  TBL_ALT_G = rgb_g(::ase::colors::DOCS_TABLE_ROW_ALT),  TBL_ALT_B = rgb_b(::ase::colors::DOCS_TABLE_ROW_ALT);

// Callout border colours (DOCS_CALLOUT_*_BORDER).
constexpr double INFO_R = rgb_r(::ase::colors::DOCS_CALLOUT_INFO_BORDER), INFO_G = rgb_g(::ase::colors::DOCS_CALLOUT_INFO_BORDER), INFO_B = rgb_b(::ase::colors::DOCS_CALLOUT_INFO_BORDER);
constexpr double WARN_R = rgb_r(::ase::colors::DOCS_CALLOUT_WARN_BORDER), WARN_G = rgb_g(::ase::colors::DOCS_CALLOUT_WARN_BORDER), WARN_B = rgb_b(::ase::colors::DOCS_CALLOUT_WARN_BORDER);
constexpr double TIP_R  = rgb_r(::ase::colors::DOCS_CALLOUT_TIP_BORDER),  TIP_G  = rgb_g(::ase::colors::DOCS_CALLOUT_TIP_BORDER),  TIP_B  = rgb_b(::ase::colors::DOCS_CALLOUT_TIP_BORDER);
constexpr double NOTE_R = rgb_r(::ase::colors::DOCS_CALLOUT_NOTE_BORDER), NOTE_G = rgb_g(::ase::colors::DOCS_CALLOUT_NOTE_BORDER), NOTE_B = rgb_b(::ase::colors::DOCS_CALLOUT_NOTE_BORDER);

// ── Spacing constants — web 1:1 (cmsViewer.tsx) ────────────────────
//   <p>     mb-2.5 = 10
//   code/table mt-3 mb-5 (12/20)
//   <hr>    my-6 = 24 each side
//   <ul/ol> mt-4 mb-2.5 ml-3 (16/10/12)

constexpr double PARA_SPACING   = 10.0;  // mb-2.5
constexpr double CODE_PADDING   = 8.0;
constexpr double LIST_INDENT    = 12.0;  // ml-3
constexpr double HR_SPACING     = 24.0;  // my-6 (was 16)
constexpr double CALLOUT_PAD    = 12.0;

// Heading margins per web (cmsViewer.tsx). Each level uses its own
// mt-/mb- pair — uniform 24/8 was wrong.
//   h1 mt-8 mb-4 (32/16)  + pb-1.5 (6) + border-bottom
//   h2 mt-7 mb-3 (28/12)
//   h3 mt-6 mb-2 (24/8)
//   h4 mt-5 mb-2 (20/8)
constexpr double H1_MT = 32.0, H1_MB = 16.0, H1_PB = 6.0;
constexpr double H2_MT = 28.0, H2_MB = 12.0;
constexpr double H3_MT = 24.0, H3_MB = 8.0;
constexpr double H4_MT = 20.0, H4_MB = 8.0;

struct HeadingSpacing { double mt, mb; };
inline HeadingSpacing heading_spacing(uint8_t level) {
    if (level == 1) return {H1_MT, H1_MB};
    if (level == 2) return {H2_MT, H2_MB};
    if (level == 3) return {H3_MT, H3_MB};
    return {H4_MT, H4_MB};
}

// SSOT-routed Pango markup fragments built once at static-init from
// ase::colors::*. Used for link spans inside math-aware inline builders
// where the open/close strings are passed as const char*.
inline const std::string& link_span_open() {
    static const std::string s = []() {
        char buf[8];
        ::ase::utils::format_hex_color(buf, sizeof(buf), ::ase::colors::PANEL_CYAN);
        return std::string{"<span foreground=\""} + buf + "\" underline=\"single\">";
    }();
    return s;
}

// ── Color dispatch (no switch) ─────────────────────────────────────

struct RGB { double r, g, b; };

RGB heading_rgb(uint8_t level) {
    if (level == 1) return {H1_R, H1_G, H1_B};
    if (level == 2) return {H2_R, H2_G, H2_B};
    if (level == 3) return {H3_R, H3_G, H3_B};
    return {H4_R, H4_G, H4_B};
}

RGB callout_rgb(uint8_t type) {
    if (type == ase::markdown::CALLOUT_INFO)    return {INFO_R, INFO_G, INFO_B};
    if (type == ase::markdown::CALLOUT_WARNING) return {WARN_R, WARN_G, WARN_B};
    if (type == ase::markdown::CALLOUT_TIP)     return {TIP_R, TIP_G, TIP_B};
    if (type == ase::markdown::CALLOUT_NOTE)    return {NOTE_R, NOTE_G, NOTE_B};
    return {MUTED_R, MUTED_G, MUTED_B};
}

void set_rgb(const Cairo::RefPtr<Cairo::Context>& cr, const RGB& c) {
    cr->set_source_rgb(c.r, c.g, c.b);
}

// ── Cairo dash helpers (cairomm-1.16 set_dash takes std::vector — bypass
// via the C cairo API to keep the source vector-free). ────────────────

void set_dash_2(const Cairo::RefPtr<Cairo::Context>& cr, double a, double b) {
    double dashes[2] = {a, b};
    cairo_set_dash(cr->cobj(), dashes, 2, 0.0);
}

void unset_dash(const Cairo::RefPtr<Cairo::Context>& cr) {
    cairo_set_dash(cr->cobj(), nullptr, 0, 0.0);
}

// Inline markup helpers (build_inline_markup / build_inline_children /
// append_escaped / collect_plain_text / streq / cstr_len) live in
// vwr_rndr_inline.hpp and are shared with every DSL renderer.

using ::ase::viewer::render::build_inline_markup;
using ::ase::viewer::render::build_inline_children;

inline void set_layout_markup(const Glib::RefPtr<Pango::Layout>& layout,
                              const std::string& markup) {
    layout->set_markup(markup);
}

// ── Inline math layout support ─────────────────────────────────────
//
// Paragraphs that contain NODE_MATH_INLINE children render text via the
// shared Pango markup builder, but inline math expressions are inserted
// as Unicode OBJECT REPLACEMENT CHARACTER (U+FFFC, 3 UTF-8 bytes) and
// reserved as Pango shape attributes with the bounding box returned by
// ase::adp::microtex::measure_math. After Pango lays out the paragraph, the
// position of every U+FFFC is queried via pango_layout_index_to_pos and
// the actual MicroTeX glyphs are rendered at that exact baseline-aligned
// pixel position. Result: full LaTeX inline math inside flowing text.

constexpr uint32_t MAX_INLINE_MATH_PER_PARAGRAPH = 64;

struct InlineMathEntry {
    const char* latex     = nullptr;
    uint32_t    latex_len = 0;
    double      width     = 0.0;
    double      height    = 0.0;
    double      depth     = 0.0;
    double      baseline  = 0.0;
};

struct InlineMathSet {
    InlineMathEntry items[MAX_INLINE_MATH_PER_PARAGRAPH];
    uint32_t count = 0;
};

bool paragraph_has_inline_math(const ase::markdown::Node* node) {
    if (node == nullptr) return false;
    using namespace ase::markdown;
    for (const ase::markdown::Node* c = node->first_child; c != nullptr; c = c->next_sibling) {
        if (c->type == NODE_MATH_INLINE) return true;
        if (paragraph_has_inline_math(c)) return true;
    }
    return false;
}

// Walks the inline children of `parent` like build_inline_children, but
// substitutes U+FFFC for every NODE_MATH_INLINE encountered and records
// the LaTeX source + length in `set` (in document order). Other inline
// nodes are emitted via the shared markup builder so emphasis / strong /
// links / NerdFont icons all keep their formatting.
void build_inline_markup_with_math(std::string& out,
                                   const ase::markdown::Node* node,
                                   InlineMathSet& set);

void build_inline_children_with_math(std::string& out,
                                     const ase::markdown::Node* parent,
                                     InlineMathSet& set) {
    if (parent == nullptr) return;
    for (const ase::markdown::Node* c = parent->first_child; c != nullptr; c = c->next_sibling) {
        build_inline_markup_with_math(out, c, set);
    }
}

void build_inline_markup_with_math(std::string& out,
                                   const ase::markdown::Node* node,
                                   InlineMathSet& set) {
    if (node == nullptr) return;
    using namespace ase::markdown;

    if (node->type == NODE_MATH_INLINE) {
        if (set.count < MAX_INLINE_MATH_PER_PARAGRAPH) {
            auto& m = set.items[set.count++];
            m.latex     = node->text;
            m.latex_len = node->text_len;
        }
        // U+FFFC OBJECT REPLACEMENT CHARACTER, 3 UTF-8 bytes.
        out.append("\xef\xbf\xbc");
        return;
    }

    // Recurse into children for container-style inline nodes (emphasis,
    // strong, link, …) so any nested math is captured too.
    if (node->type == NODE_EMPHASIS || node->type == NODE_STRONG ||
        node->type == NODE_STRIKETHROUGH || node->type == NODE_LINK) {
        const char* open  = "<i>";
        const char* close = "</i>";
        if (node->type == NODE_STRONG)             { open = "<b>"; close = "</b>"; }
        else if (node->type == NODE_STRIKETHROUGH) { open = "<s>"; close = "</s>"; }
        else if (node->type == NODE_LINK) {
            // SSOT-routed via link_span_open() — never write hex literals.
            out.append(link_span_open());
            build_inline_children_with_math(out, node, set);
            out.append("</span>");
            return;
        }
        out.append(open);
        build_inline_children_with_math(out, node, set);
        out.append(close);
        return;
    }

    // Anything else delegates to the regular inline builder which knows
    // about text/icons/wiki/glossary/xref/etc.
    build_inline_markup(out, node);
}

void render_paragraph_inline_math(RenderContext& ctx,
                                  const ase::markdown::Node* node,
                                  int content_width) {
    InlineMathSet maths;
    std::string markup;
    build_inline_children_with_math(markup, node, maths);

    // Pre-measure each math expression at the body font size.
    const double font_px = static_cast<double>(FONT_PX_BODY);
    for (uint32_t i = 0; i < maths.count; ++i) {
        const MathResult mr = measure_math(maths.items[i].latex,
                                           maths.items[i].latex_len,
                                           font_px,
                                           /* is_display = */ false);
        maths.items[i].width    = mr.width;
        maths.items[i].height   = mr.height;
        maths.items[i].depth    = mr.depth;
        maths.items[i].baseline = mr.baseline;
    }

    // Parse markup into raw text + Pango attribute list.
    char* parsed_text = nullptr;
    PangoAttrList* attrs = nullptr;
    GError* err = nullptr;
    if (!pango_parse_markup(markup.c_str(),
                            static_cast<int>(markup.size()),
                            0, &attrs, &parsed_text, nullptr, &err)) {
        if (err != nullptr) g_error_free(err);
        // Fallback to plain markup paragraph if parse fails (defensive).
        auto layout = create_body_layout(ctx.cr, content_width);
        layout->set_markup(markup);
        ctx.cr->set_source_rgb(TEXT_R, TEXT_G, TEXT_B);
        draw_layout(ctx.cr, layout, ctx.margin_left, ctx.y);
        int w = 0, h = 0;
        layout->get_pixel_size(w, h);
        ctx.y += h + PARA_SPACING;
        return;
    }

    // Walk the parsed text in order, find each U+FFFC, and attach a Pango
    // shape attribute that reserves the math expression's bounding box.
    uint32_t math_idx = 0;
    int parsed_len = 0;
    while (parsed_text[parsed_len] != '\0') ++parsed_len;

    int pos_byte = 0;
    while (pos_byte + 2 < parsed_len && math_idx < maths.count) {
        const unsigned char b0 = static_cast<unsigned char>(parsed_text[pos_byte]);
        const unsigned char b1 = static_cast<unsigned char>(parsed_text[pos_byte + 1]);
        const unsigned char b2 = static_cast<unsigned char>(parsed_text[pos_byte + 2]);
        if (b0 == 0xef && b1 == 0xbf && b2 == 0xbc) {
            const auto& m = maths.items[math_idx];
            const int w_pango = static_cast<int>(m.width  * PANGO_SCALE);
            const int h_pango = static_cast<int>(m.height * PANGO_SCALE);
            const int d_pango = static_cast<int>(m.depth  * PANGO_SCALE);

            PangoRectangle ink;
            ink.x = 0;
            ink.y = -(h_pango - d_pango);
            ink.width  = w_pango;
            ink.height = h_pango;

            PangoRectangle logical = ink;

            PangoAttribute* shape_attr = pango_attr_shape_new(&ink, &logical);
            shape_attr->start_index = pos_byte;
            shape_attr->end_index   = pos_byte + 3;
            pango_attr_list_insert(attrs, shape_attr);

            ++math_idx;
            pos_byte += 3;
            continue;
        }
        ++pos_byte;
    }

    auto layout = create_body_layout(ctx.cr, content_width);
    PangoLayout* clayout = layout->gobj();
    pango_layout_set_text(clayout, parsed_text, parsed_len);
    pango_layout_set_attributes(clayout, attrs);
    pango_attr_list_unref(attrs);

    int layout_w = 0, layout_h = 0;
    layout->get_pixel_size(layout_w, layout_h);

    // Draw the text first, then overlay each math expression at its
    // exact laid-out byte position.
    ctx.cr->set_source_rgb(TEXT_R, TEXT_G, TEXT_B);
    draw_layout(ctx.cr, layout, ctx.margin_left, ctx.y);

    math_idx = 0;
    pos_byte = 0;
    while (pos_byte + 2 < parsed_len && math_idx < maths.count) {
        const unsigned char b0 = static_cast<unsigned char>(parsed_text[pos_byte]);
        const unsigned char b1 = static_cast<unsigned char>(parsed_text[pos_byte + 1]);
        const unsigned char b2 = static_cast<unsigned char>(parsed_text[pos_byte + 2]);
        if (b0 == 0xef && b1 == 0xbf && b2 == 0xbc) {
            PangoRectangle pos_rect;
            pango_layout_index_to_pos(clayout, pos_byte, &pos_rect);
            const double mx = ctx.margin_left + pos_rect.x / static_cast<double>(PANGO_SCALE);
            const double my = ctx.y           + pos_rect.y / static_cast<double>(PANGO_SCALE);
            const auto& m = maths.items[math_idx];
            (void)render_math(ctx.cr, m.latex, m.latex_len, mx, my, font_px, /* is_display = */ false);
            ++math_idx;
            pos_byte += 3;
            continue;
        }
        ++pos_byte;
    }

    g_free(parsed_text);
    ctx.y += layout_h + PARA_SPACING;
}

// ── Per-node renderers ─────────────────────────────────────────────

void render_heading(RenderContext& ctx, const ase::markdown::Node* node, int content_width) {
    const HeadingSpacing sp = heading_spacing(node->heading_level);
    ctx.y += sp.mt;
    auto layout = create_heading_layout(ctx.cr, content_width, node->heading_level);
    std::string markup;
    build_inline_children(markup, node);
    set_layout_markup(layout, markup);
    set_rgb(ctx.cr, heading_rgb(node->heading_level));
    draw_layout(ctx.cr, layout, ctx.margin_left, ctx.y);
    int w = 0, h = 0;
    layout->get_pixel_size(w, h);
    ctx.y += h;
    if (node->heading_level == 1) {
        // Web h1: pb-1.5 (6) + border-bottom 1 px BORDER_A30.
        ctx.y += H1_PB;
        ctx.cr->set_source_rgba(rgb_r(::ase::colors::BORDER_A30),
                                rgb_g(::ase::colors::BORDER_A30),
                                rgb_b(::ase::colors::BORDER_A30),
                                rgb_a(::ase::colors::BORDER_A30));
        ctx.cr->set_line_width(1);
        ctx.cr->move_to(ctx.margin_left, ctx.y);
        ctx.cr->line_to(ctx.width - ctx.margin_right, ctx.y);
        ctx.cr->stroke();
    }
    ctx.y += sp.mb;
}

void render_paragraph(RenderContext& ctx, const ase::markdown::Node* node, int content_width) {
    if (paragraph_has_inline_math(node)) {
        render_paragraph_inline_math(ctx, node, content_width);
        return;
    }
    auto layout = create_body_layout(ctx.cr, content_width);
    std::string markup;
    build_inline_children(markup, node);
    set_layout_markup(layout, markup);
    ctx.cr->set_source_rgb(TEXT_R, TEXT_G, TEXT_B);
    draw_layout(ctx.cr, layout, ctx.margin_left, ctx.y);
    int w = 0, h = 0;
    layout->get_pixel_size(w, h);
    ctx.y += h + PARA_SPACING;
}

void render_code_block(RenderContext& ctx, const ase::markdown::Node* node, int content_width) {
    ctx.y += 4;
    auto layout = create_code_layout(ctx.cr, content_width - 2 * static_cast<int>(CODE_PADDING));

    // Run the code through tree-sitter via vwr_rndr_synt to produce a Pango
    // markup string with span foreground colours per token. If the language
    // is unsupported the helper returns the plain (escaped) text and we fall
    // back to set_text — both branches share the same Pango layout.
    if (node->text != nullptr && node->text_len > 0) {
        const std::string markup = syntax_highlight(node->text, node->text_len, node->language);
        layout->set_markup(markup);
    } else {
        layout->set_text("");
    }

    int w = 0, h = 0;
    layout->get_pixel_size(w, h);
    ctx.cr->set_source_rgb(CODE_BG_R, CODE_BG_G, CODE_BG_B);
    ctx.cr->rectangle(ctx.margin_left, ctx.y, content_width, h + 2 * CODE_PADDING + 18);
    ctx.cr->fill();
    if (node->language != nullptr) {
        auto label_layout = Pango::Layout::create(ctx.cr);
        Pango::FontDescription fd;
        fd.set_family("Fira Code");
        fd.set_size(9 * Pango::SCALE);
        label_layout->set_font_description(fd);
        label_layout->set_text(node->language);
        ctx.cr->set_source_rgb(MUTED_R, MUTED_G, MUTED_B);
        draw_layout(ctx.cr, label_layout, ctx.margin_left + CODE_PADDING, ctx.y + 4);
    }
    ctx.cr->set_source_rgb(TEXT_R, TEXT_G, TEXT_B);
    draw_layout(ctx.cr, layout, ctx.margin_left + CODE_PADDING, ctx.y + 18 + CODE_PADDING);
    ctx.y += h + 2 * CODE_PADDING + 18 + PARA_SPACING;
}

void render_diff_block(RenderContext& ctx, const ase::markdown::Node* node, int content_width) {
    ctx.y += 4;
    std::string text;
    if (node->text != nullptr && node->text_len > 0) text.assign(node->text, node->text_len);

    constexpr double LINE_H = 16.0;
    int line_count = 1;
    for (uint32_t i = 0; i < text.size(); ++i) if (text[i] == '\n') ++line_count;
    double block_h = line_count * LINE_H + 2 * CODE_PADDING;

    ctx.cr->set_source_rgb(CODE_BG_R, CODE_BG_G, CODE_BG_B);
    ctx.cr->rectangle(ctx.margin_left, ctx.y, content_width, block_h);
    ctx.cr->fill();

    double ly = ctx.y + CODE_PADDING;
    uint32_t i = 0;
    while (i < text.size()) {
        uint32_t line_start = i;
        while (i < text.size() && text[i] != '\n') ++i;
        std::string line = text.substr(line_start, i - line_start);
        if (i < text.size()) ++i;

        if (!line.empty()) {
            if (line[0] == '+') {
                ctx.cr->set_source_rgba(rgb_r(::ase::colors::DIFF_ADD_BG),
                                        rgb_g(::ase::colors::DIFF_ADD_BG),
                                        rgb_b(::ase::colors::DIFF_ADD_BG),
                                        rgb_a(::ase::colors::DIFF_ADD_BG));
                ctx.cr->rectangle(ctx.margin_left, ly - 1, content_width, LINE_H);
                ctx.cr->fill();
                ctx.cr->set_source_rgb(rgb_r(::ase::colors::DIFF_ADD_FG),
                                       rgb_g(::ase::colors::DIFF_ADD_FG),
                                       rgb_b(::ase::colors::DIFF_ADD_FG));
            } else if (line[0] == '-') {
                ctx.cr->set_source_rgba(rgb_r(::ase::colors::DIFF_REMOVE_BG),
                                        rgb_g(::ase::colors::DIFF_REMOVE_BG),
                                        rgb_b(::ase::colors::DIFF_REMOVE_BG),
                                        rgb_a(::ase::colors::DIFF_REMOVE_BG));
                ctx.cr->rectangle(ctx.margin_left, ly - 1, content_width, LINE_H);
                ctx.cr->fill();
                ctx.cr->set_source_rgb(rgb_r(::ase::colors::DIFF_REMOVE_FG),
                                       rgb_g(::ase::colors::DIFF_REMOVE_FG),
                                       rgb_b(::ase::colors::DIFF_REMOVE_FG));
            } else if (line[0] == '[') {
                ctx.cr->set_source_rgb(MUTED_R, MUTED_G, MUTED_B);
            } else {
                ctx.cr->set_source_rgb(TEXT_R, TEXT_G, TEXT_B);
            }
        }

        auto line_layout = Pango::Layout::create(ctx.cr);
        Pango::FontDescription fd;
        fd.set_family("Fira Code");
        fd.set_size(10 * Pango::SCALE);
        line_layout->set_font_description(fd);
        line_layout->set_text(line);
        draw_layout(ctx.cr, line_layout, ctx.margin_left + CODE_PADDING, ly);
        ly += LINE_H;
    }

    ctx.y += block_h + PARA_SPACING;
}

void render_callout(RenderContext& ctx, const ase::markdown::Node* node, int content_width) {
    ctx.y += 4;
    auto layout = create_body_layout(ctx.cr, content_width - 28);
    std::string markup;
    // Walk paragraph children of the callout (callouts wrap a paragraph).
    const ase::markdown::Node* c = node->first_child;
    while (c != nullptr) {
        if (c->type == ase::markdown::NODE_PARAGRAPH) {
            build_inline_children(markup, c);
        } else {
            build_inline_markup(markup, c);
        }
        if (c->next_sibling != nullptr) markup.push_back('\n');
        c = c->next_sibling;
    }
    set_layout_markup(layout, markup);
    int w = 0, h = 0;
    layout->get_pixel_size(w, h);

    auto cc = callout_rgb(node->callout_type);
    ctx.cr->set_source_rgba(cc.r, cc.g, cc.b, 0.08);
    ctx.cr->rectangle(ctx.margin_left, ctx.y, content_width, h + 2 * CALLOUT_PAD);
    ctx.cr->fill();

    ctx.cr->set_source_rgb(cc.r, cc.g, cc.b);
    ctx.cr->set_line_width(3);
    ctx.cr->move_to(ctx.margin_left, ctx.y);
    ctx.cr->line_to(ctx.margin_left, ctx.y + h + 2 * CALLOUT_PAD);
    ctx.cr->stroke();

    ctx.cr->set_source_rgb(TEXT_R, TEXT_G, TEXT_B);
    draw_layout(ctx.cr, layout, ctx.margin_left + 16, ctx.y + CALLOUT_PAD);
    ctx.y += h + 2 * CALLOUT_PAD + PARA_SPACING;
}

void render_list(RenderContext& ctx, const ase::markdown::Node* node, int content_width);
void render_blockquote(RenderContext& ctx, const ase::markdown::Node* node, int content_width);
void render_thematic_break(RenderContext& ctx);
void render_table(RenderContext& ctx, const ase::markdown::Node* node, int content_width);
void render_mermaid_block(RenderContext& ctx, const ase::markdown::Node* node, int content_width);
void render_math_block(RenderContext& ctx, const ase::markdown::Node* node, int content_width);
void render_image(RenderContext& ctx, const ase::markdown::Node* node, int content_width);

void render_list(RenderContext& ctx, const ase::markdown::Node* node, int content_width) {
    const ase::markdown::Node* item = node->first_child;
    int index = static_cast<int>(node->list_start);
    while (item != nullptr) {
        auto layout = create_body_layout(ctx.cr, content_width - static_cast<int>(LIST_INDENT));
        std::string markup;
        // Walk item children — first paragraph contains the inline content.
        const ase::markdown::Node* ic = item->first_child;
        while (ic != nullptr) {
            if (ic->type == ase::markdown::NODE_PARAGRAPH) build_inline_children(markup, ic);
            else build_inline_markup(markup, ic);
            ic = ic->next_sibling;
        }
        set_layout_markup(layout, markup);

        auto bullet_layout = Pango::Layout::create(ctx.cr);
        Pango::FontDescription fd;
        fd.set_family("Fira Code");
        fd.set_size(11 * Pango::SCALE);
        bullet_layout->set_font_description(fd);
        if (node->list_ordered != 0) {
            bullet_layout->set_text(std::to_string(index++) + ".");
        } else {
            bullet_layout->set_text("→");
        }
        ctx.cr->set_source_rgb(MUTED_R, MUTED_G, MUTED_B);
        draw_layout(ctx.cr, bullet_layout, ctx.margin_left, ctx.y);

        ctx.cr->set_source_rgb(TEXT_R, TEXT_G, TEXT_B);
        draw_layout(ctx.cr, layout, ctx.margin_left + LIST_INDENT, ctx.y);
        int w = 0, h = 0;
        layout->get_pixel_size(w, h);
        ctx.y += h + 2;
        item = item->next_sibling;
    }
    ctx.y += PARA_SPACING;
}

void render_blockquote(RenderContext& ctx, const ase::markdown::Node* node, int content_width) {
    ctx.y += 4;
    auto layout = create_body_layout(ctx.cr, content_width - 20);
    std::string markup;
    const ase::markdown::Node* c = node->first_child;
    while (c != nullptr) {
        if (c->type == ase::markdown::NODE_PARAGRAPH) build_inline_children(markup, c);
        else build_inline_markup(markup, c);
        if (c->next_sibling != nullptr) markup.push_back('\n');
        c = c->next_sibling;
    }
    set_layout_markup(layout, markup);
    int w = 0, h = 0;
    layout->get_pixel_size(w, h);

    ctx.cr->set_source_rgba(1, 1, 1, 0.02);
    ctx.cr->rectangle(ctx.margin_left, ctx.y, content_width, h + 16);
    ctx.cr->fill();

    ctx.cr->set_source_rgba(1, 1, 1, 0.08);
    ctx.cr->set_line_width(2);
    ctx.cr->move_to(ctx.margin_left, ctx.y);
    ctx.cr->line_to(ctx.margin_left, ctx.y + h + 16);
    ctx.cr->stroke();

    ctx.cr->set_source_rgb(MUTED_R, MUTED_G, MUTED_B);
    draw_layout(ctx.cr, layout, ctx.margin_left + 12, ctx.y + 8);
    ctx.y += h + 16 + PARA_SPACING;
}

void render_thematic_break(RenderContext& ctx) {
    ctx.y += HR_SPACING;
    ctx.cr->set_source_rgba(1, 1, 1, 0.05);
    ctx.cr->move_to(ctx.margin_left, ctx.y);
    ctx.cr->line_to(ctx.width - ctx.margin_right, ctx.y);
    ctx.cr->stroke();
    ctx.y += HR_SPACING;
}

void render_table(RenderContext& ctx, const ase::markdown::Node* node, int content_width) {
    constexpr double CELL_PAD = 6.0;
    const ase::markdown::Node* row = node->first_child;
    bool is_header = true;
    int row_index = 0;

    while (row != nullptr) {
        int cell_count = 0;
        const ase::markdown::Node* cc = row->first_child;
        while (cc != nullptr) { ++cell_count; cc = cc->next_sibling; }
        int cell_width = (cell_count > 0) ? content_width / cell_count : content_width;

        double max_h = 0;
        cc = row->first_child;
        while (cc != nullptr) {
            auto layout = create_body_layout(ctx.cr, cell_width - 2 * static_cast<int>(CELL_PAD));
            if (is_header) {
                auto fd = layout->get_font_description();
                fd.set_weight(Pango::Weight::BOLD);
                layout->set_font_description(fd);
            }
            std::string markup;
            build_inline_children(markup, cc);
            set_layout_markup(layout, markup);
            int w = 0, h = 0;
            layout->get_pixel_size(w, h);
            if (h > max_h) max_h = h;
            cc = cc->next_sibling;
        }
        double row_h = max_h + 2 * CELL_PAD;

        if (is_header) {
            ctx.cr->set_source_rgb(TBL_HD_R, TBL_HD_G, TBL_HD_B);
        } else if (row_index % 2 == 0) {
            ctx.cr->set_source_rgb(TBL_BG_R, TBL_BG_G, TBL_BG_B);
        } else {
            ctx.cr->set_source_rgb(TBL_ALT_R, TBL_ALT_G, TBL_ALT_B);
        }
        ctx.cr->rectangle(ctx.margin_left, ctx.y, content_width, row_h);
        ctx.cr->fill();

        double x = ctx.margin_left;
        cc = row->first_child;
        while (cc != nullptr) {
            auto layout = create_body_layout(ctx.cr, cell_width - 2 * static_cast<int>(CELL_PAD));
            if (is_header) {
                auto fd = layout->get_font_description();
                fd.set_weight(Pango::Weight::BOLD);
                layout->set_font_description(fd);
            }
            std::string markup;
            build_inline_children(markup, cc);
            set_layout_markup(layout, markup);

            if (is_header) ctx.cr->set_source_rgb(H2_R, H2_G, H2_B);
            else ctx.cr->set_source_rgb(TEXT_R, TEXT_G, TEXT_B);
            draw_layout(ctx.cr, layout, x + CELL_PAD, ctx.y + CELL_PAD);

            ctx.cr->set_source_rgba(1, 1, 1, 0.06);
            ctx.cr->set_line_width(0.5);
            ctx.cr->rectangle(x, ctx.y, cell_width, row_h);
            ctx.cr->stroke();

            x += cell_width;
            cc = cc->next_sibling;
        }

        ctx.y += row_h;
        if (!is_header) ++row_index;
        is_header = false;
        row = row->next_sibling;
    }
    ctx.y += PARA_SPACING;
}

void render_mermaid_block(RenderContext& ctx, const ase::markdown::Node* node, int content_width) {
    ctx.y += 4;
    if (node->text != nullptr && node->text_len > 0) {
        double h = render_mermaid(ctx.cr, node->text, node->text_len,
                                  ctx.margin_left, ctx.y, content_width);
        ctx.y += h + PARA_SPACING;
    } else {
        ctx.y += PARA_SPACING;
    }
}

void render_math_block(RenderContext& ctx, const ase::markdown::Node* node, int content_width) {
    ctx.y += 4;
    if (node->text != nullptr && node->text_len > 0) {
        MathResult r = render_math(ctx.cr, node->text, node->text_len,
                                   ctx.margin_left, ctx.y,
                                   18.0, /* is_display = */ true);
        double drawn_h = (r.height > 0) ? r.height : 24.0;
        (void)content_width;
        ctx.y += drawn_h + PARA_SPACING;
    } else {
        ctx.y += PARA_SPACING;
    }
}

void render_asemath_plot(RenderContext& ctx, const ase::markdown::Node* node, int content_width) {
    ctx.y += 4;
    if (node->text != nullptr && node->text_len > 0) {
        const double h = render_ase_math_plot(ctx.cr, node->text, node->text_len,
                                              ctx.margin_left, ctx.y, content_width);
        ctx.y += h + PARA_SPACING;
    } else {
        ctx.y += PARA_SPACING;
    }
}

void render_image(RenderContext& ctx, const ase::markdown::Node* node, int content_width) {
    ctx.y += 4;
    if (node->url == nullptr) {
        ctx.y += PARA_SPACING;
        return;
    }
    const std::string path = resolve_image_path(node->url);
    const double h = ::ase::imgcache::render(
        ctx.cr, path, ctx.margin_left, ctx.y, content_width);
    ctx.y += h + PARA_SPACING;
}

}  // namespace

// ── Public dispatcher (no switch, if/else ladder) ──────────────────

void render_node(RenderContext& ctx, const ase::markdown::Node* node) {
    if (node == nullptr) return;
    using namespace ase::markdown;

    int content_width = ctx.width - ctx.margin_left - ctx.margin_right;

    if (node->type == NODE_HEADING) {
        render_heading(ctx, node, content_width); return;
    }
    if (node->type == NODE_PARAGRAPH) {
        render_paragraph(ctx, node, content_width); return;
    }
    if (node->type == NODE_CODE_BLOCK) {
        render_code_block(ctx, node, content_width); return;
    }
    if (node->type == NODE_DIFF_BLOCK) {
        render_diff_block(ctx, node, content_width); return;
    }
    if (node->type == NODE_MERMAID_BLOCK) {
        render_mermaid_block(ctx, node, content_width); return;
    }
    if (node->type == NODE_MATH_DISPLAY) {
        render_math_block(ctx, node, content_width); return;
    }
    if (node->type == NODE_ASEMATH_BLOCK) {
        render_asemath_plot(ctx, node, content_width); return;
    }
    if (node->type == NODE_SVGBOB_BLOCK) {
        // svgbob fallback: render as plain code block until a dedicated
        // renderer ships.
        render_code_block(ctx, node, content_width); return;
    }
    if (node->type == NODE_CALLOUT) {
        render_callout(ctx, node, content_width); return;
    }
    if (node->type == NODE_LIST) {
        render_list(ctx, node, content_width); return;
    }
    if (node->type == NODE_BLOCKQUOTE) {
        render_blockquote(ctx, node, content_width); return;
    }
    if (node->type == NODE_THEMATIC_BREAK) {
        render_thematic_break(ctx); return;
    }
    if (node->type == NODE_TABLE) {
        render_table(ctx, node, content_width); return;
    }
    if (node->type == NODE_IMAGE) {
        render_image(ctx, node, content_width); return;
    }
    if (node->type == NODE_BLOCK_DIRECTIVE ||
        node->type == NODE_LEAF_DIRECTIVE) {
        dsl::render_directive(ctx, node);
        return;
    }
    if (node->type == NODE_TEXT_DIRECTIVE) {
        // Top-level text directives only happen when a paragraph was
        // mutated by pass_inline_extensions into its first inline child's
        // type (parser markdown_prs_drct.cpp ~628). Render as an inline
        // paragraph — never dispatch to the DSL block dispatcher which
        // would draw a purple placeholder box around a single line of
        // text.
        auto layout = create_body_layout(ctx.cr, content_width);
        std::string markup;
        build_inline_markup(markup, node);
        set_layout_markup(layout, markup);
        ctx.cr->set_source_rgb(TEXT_R, TEXT_G, TEXT_B);
        draw_layout(ctx.cr, layout, ctx.margin_left, ctx.y);
        int w = 0, h = 0;
        layout->get_pixel_size(w, h);
        ctx.y += h + PARA_SPACING;
        return;
    }
    if (node->type == NODE_NERDFONT_ICON ||
        node->type == NODE_WIKI_LINK     ||
        node->type == NODE_GLOSSARY_TERM ||
        node->type == NODE_CROSS_REF     ||
        node->type == NODE_VERSION_INSERT||
        node->type == NODE_MATH_INLINE) {
        // Stray inline node at block level — wrap in a one-line layout so
        // it still renders instead of being silently dropped.
        auto layout = create_body_layout(ctx.cr, content_width);
        std::string markup;
        build_inline_markup(markup, node);
        set_layout_markup(layout, markup);
        ctx.cr->set_source_rgb(TEXT_R, TEXT_G, TEXT_B);
        draw_layout(ctx.cr, layout, ctx.margin_left, ctx.y);
        int w = 0, h = 0;
        layout->get_pixel_size(w, h);
        ctx.y += h + PARA_SPACING;
        return;
    }

    // Unknown container — recurse into children.
    const ase::markdown::Node* c = node->first_child;
    while (c != nullptr) {
        render_node(ctx, c);
        c = c->next_sibling;
    }
}

double render_document(RenderContext& ctx, const ase::markdown::Document& doc) {
    ctx.y = ctx.margin_top;
    ctx.next_tabs_index = 0;
    ctx.next_accordion_index = 0;
    if (ctx.regions != nullptr) ctx.regions->count = 0;
    if (doc.root != nullptr) {
        const ase::markdown::Node* child = doc.root->first_child;
        while (child != nullptr) {
            render_node(ctx, child);
            child = child->next_sibling;
        }
    }
    return ctx.y + ctx.margin_top;
}

}  // namespace ase::viewer::render
