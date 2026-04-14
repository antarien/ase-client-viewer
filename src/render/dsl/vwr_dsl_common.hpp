#pragma once

/**
 * Shared utilities for DSL directive renderers.
 *
 * Design SSOT — all colors and tokens come from the generated headers:
 *   colors.hpp         (from sha-web-styles/src/colors.ts → ase::colors::*)
 *   design_tokens.hpp  (from sha-web-styles/src/theme.css → ase::theme::*)
 * Never hardcode hex, pixel or font values here — regenerate with
 *   node sha-web-console/generate-colors.cjs
 *   node sha-web-console/generate-design-tokens.cjs
 *
 * Inline text — vwr_rndr_inline.hpp (shared with the document dispatcher)
 *
 * No std::vector / std::strcmp / std::sstream — directive children are
 * walked through the iterator-style helpers (first_child_named /
 * next_child_named) and inline content is built into a Pango markup
 * string via build_inline_children().
 *
 * @module      ase-client-viewer
 * @layer       5 (Clients)
 */

#include "../vwr_rndr_doc.hpp"
#include "../vwr_rndr_text.hpp"
#include "../vwr_rndr_inline.hpp"

#include <ase/markdown/ast.hpp>
#include <ase/markdown/types.hpp>
#include <ase/utils/strops.hpp>

// Generated color SSOT (sha-web-console/generated/). Include-path is
// provided by clients/ase-client-viewer/CMakeLists.txt. Per-block
// renderers that need design_tokens.hpp (spacing/font sizes) include it
// themselves when they consume a token.
#include "colors.hpp"

#include <cmath>
#include <cstdint>
#include <string>

namespace ase::viewer::render::dsl {

// ── Color SSOT — derived from ase::colors::* (generated from colors.ts)

// Extract normalised R/G/B/A channels from a 0xAARRGGBB constant so each
// Cairo cr->set_source_rgb / _rgba call can consume component-wise values
// while the underlying hex remains the single source of truth. Generated
// rgba() colors from colors.ts (e.g. BORDER_A20, CMS_CALLOUT_*_BG) carry
// their alpha in the high byte and must be drawn with set_source_rgba.
constexpr double rgb_a(uint32_t argb) { return static_cast<double>((argb >> 24) & 0xFFu) / 255.0; }
constexpr double rgb_r(uint32_t argb) { return static_cast<double>((argb >> 16) & 0xFFu) / 255.0; }
constexpr double rgb_g(uint32_t argb) { return static_cast<double>((argb >>  8) & 0xFFu) / 255.0; }
constexpr double rgb_b(uint32_t argb) { return static_cast<double>((argb      ) & 0xFFu) / 255.0; }

// Format a 0xAARRGGBB color constant as a "#RRGGBB" string suitable for
// Pango markup foreground/background attributes. Routes through
// ase::utils::format_hex_color so the underlying hex remains SSOT —
// callers must NEVER write literal "#xxxxxx" strings into markup.
inline std::string color_hex(uint32_t argb) {
    char buf[8];
    ::ase::utils::format_hex_color(buf, sizeof(buf), argb);
    return std::string{buf};
}

// Heading palette — H1..H4 semantic roles from the web styleguide.
constexpr double CYAN_R   = rgb_r(::ase::colors::DOCS_H1), CYAN_G   = rgb_g(::ase::colors::DOCS_H1), CYAN_B   = rgb_b(::ase::colors::DOCS_H1);
constexpr double CHART_R  = rgb_r(::ase::colors::DOCS_H2), CHART_G  = rgb_g(::ase::colors::DOCS_H2), CHART_B  = rgb_b(::ase::colors::DOCS_H2);
constexpr double ORANGE_R = rgb_r(::ase::colors::DOCS_H3), ORANGE_G = rgb_g(::ase::colors::DOCS_H3), ORANGE_B = rgb_b(::ase::colors::DOCS_H3);
constexpr double PURPLE_R = rgb_r(::ase::colors::DOCS_H4), PURPLE_G = rgb_g(::ase::colors::DOCS_H4), PURPLE_B = rgb_b(::ase::colors::DOCS_H4);

// Callout accent borders.
constexpr double GREEN_R  = rgb_r(::ase::colors::DOCS_CALLOUT_TIP_BORDER),  GREEN_G  = rgb_g(::ase::colors::DOCS_CALLOUT_TIP_BORDER),  GREEN_B  = rgb_b(::ase::colors::DOCS_CALLOUT_TIP_BORDER);
constexpr double AMBER_R  = rgb_r(::ase::colors::DOCS_CALLOUT_WARN_BORDER), AMBER_G  = rgb_g(::ase::colors::DOCS_CALLOUT_WARN_BORDER), AMBER_B  = rgb_b(::ase::colors::DOCS_CALLOUT_WARN_BORDER);
constexpr double NOTE_R   = rgb_r(::ase::colors::DOCS_CALLOUT_NOTE_BORDER), NOTE_G   = rgb_g(::ase::colors::DOCS_CALLOUT_NOTE_BORDER), NOTE_B   = rgb_b(::ase::colors::DOCS_CALLOUT_NOTE_BORDER);
constexpr double INFO_R   = rgb_r(::ase::colors::DOCS_CALLOUT_INFO_BORDER), INFO_G   = rgb_g(::ase::colors::DOCS_CALLOUT_INFO_BORDER), INFO_B   = rgb_b(::ase::colors::DOCS_CALLOUT_INFO_BORDER);
constexpr double RED_R    = rgb_r(::ase::colors::PANEL_RED),                RED_G    = rgb_g(::ase::colors::PANEL_RED),                RED_B    = rgb_b(::ase::colors::PANEL_RED);

// Text palette.
constexpr double TEXT_R   = rgb_r(::ase::colors::TEXT_PRIMARY),     TEXT_G   = rgb_g(::ase::colors::TEXT_PRIMARY),     TEXT_B   = rgb_b(::ase::colors::TEXT_PRIMARY);
constexpr double MUTED_R  = rgb_r(::ase::colors::TEXT_PANEL_MUTED), MUTED_G  = rgb_g(::ase::colors::TEXT_PANEL_MUTED), MUTED_B  = rgb_b(::ase::colors::TEXT_PANEL_MUTED);
constexpr double DOCSFG_R = rgb_r(::ase::colors::TEXT_DOCS_FG),     DOCSFG_G = rgb_g(::ase::colors::TEXT_DOCS_FG),     DOCSFG_B = rgb_b(::ase::colors::TEXT_DOCS_FG);

// DSL block surfaces.
constexpr double SURFACE_R     = rgb_r(::ase::colors::DOCS_DSL_BG),        SURFACE_G     = rgb_g(::ase::colors::DOCS_DSL_BG),        SURFACE_B     = rgb_b(::ase::colors::DOCS_DSL_BG);
constexpr double SURFACE_BRT_R = rgb_r(::ase::colors::DOCS_TABLE_ROW_ALT), SURFACE_BRT_G = rgb_g(::ase::colors::DOCS_TABLE_ROW_ALT), SURFACE_BRT_B = rgb_b(::ase::colors::DOCS_TABLE_ROW_ALT);

// ── Layout spacing — 1:1 web parity (Tailwind 4) ───────────────────
//
// These two constants are literal pixel values taken directly from the web
// CSS and do not map cleanly onto the 4/8/16/24/32/48 design-token scale
// in ase::theme::*. Tailwind's intermediate steps (mb-2.5 = 10 px,
// py-3 = 12 px) are the actual rhythm the web renderer uses for docs.
//
// PARA_SPACING — 10 px, matches <p> margin-bottom in the web CMS viewer
//                (cmsViewer.tsx, `mb-2.5`).
// BLOCK_PAD    — 12 px, matches py-3 content padding used by accordion,
//                tabs-content, terminal and similar "flow-text inside a
//                block" renderers. Blocks with their own padding (callout
//                py-4, stats/columns p-4, hero p-8) must override this
//                locally — do NOT raise this shared default.
//
// If the web ever switches to pure 4/8/16 spacing, swap these to
// ase::theme::space_* — until then literal values preserve 1:1 parity.
constexpr double PARA_SPACING = 10.0;
constexpr double BLOCK_PAD    = 12.0;

// ── Attribute & child accessors ────────────────────────────────────

inline bool name_is(const ase::markdown::Node* node, const char* name) {
    if (node == nullptr) return false;
    return ::ase::viewer::render::streq(node->directive_name, name);
}

inline const char* get_attr(const ase::markdown::Node* node, const char* key) {
    if (node == nullptr || node->attrs == nullptr) return nullptr;
    for (uint16_t i = 0; i < node->attr_count; ++i) {
        if (::ase::viewer::render::streq(node->attrs[i].key, key)) {
            return node->attrs[i].value;
        }
    }
    return nullptr;
}

inline int get_attr_int(const ase::markdown::Node* node, const char* key, int fallback) {
    const char* attr_val = get_attr(node, key);
    if (attr_val == nullptr) return fallback;
    int result = 0;
    for (int i = 0; attr_val[i] >= '0' && attr_val[i] <= '9'; ++i) {
        result = result * 10 + (attr_val[i] - '0');
    }
    return result > 0 ? result : fallback;
}

// Iterator-style children walker — replaces std::vector get_children().
// Pattern:
//   for (const Node* child = first_child_named(node, "card"); child != nullptr;
//        child = next_child_named(child, "card")) { … }
inline const ase::markdown::Node* first_child_named(const ase::markdown::Node* parent, const char* name) {
    if (parent == nullptr) return nullptr;
    for (const ase::markdown::Node* c = parent->first_child; c != nullptr; c = c->next_sibling) {
        if (::ase::viewer::render::streq(c->directive_name, name)) return c;
    }
    return nullptr;
}

inline const ase::markdown::Node* next_child_named(const ase::markdown::Node* current, const char* name) {
    if (current == nullptr) return nullptr;
    for (const ase::markdown::Node* c = current->next_sibling; c != nullptr; c = c->next_sibling) {
        if (::ase::viewer::render::streq(c->directive_name, name)) return c;
    }
    return nullptr;
}

inline uint32_t count_children_named(const ase::markdown::Node* parent, const char* name) {
    uint32_t n = 0;
    for (const ase::markdown::Node* c = first_child_named(parent, name); c != nullptr;
         c = next_child_named(c, name)) {
        ++n;
    }
    return n;
}

// ── Inline content builder shortcut ────────────────────────────────
//
// Build a Pango markup string from the inline children of a directive.
// Skips child directives so callers can render them separately.
inline std::string build_inline_content(const ase::markdown::Node* node) {
    std::string out;
    if (node == nullptr) return out;
    using namespace ase::markdown;
    for (const ase::markdown::Node* c = node->first_child; c != nullptr; c = c->next_sibling) {
        if (c->type == NODE_BLOCK_DIRECTIVE || c->type == NODE_LEAF_DIRECTIVE) continue;
        if (c->type == NODE_PARAGRAPH) {
            ::ase::viewer::render::build_inline_children(out, c);
            if (c->next_sibling != nullptr) out.push_back('\n');
            continue;
        }
        ::ase::viewer::render::build_inline_markup(out, c);
    }
    return out;
}

inline std::string get_attr_or_text(const ase::markdown::Node* node, const char* key) {
    const char* attr_val = get_attr(node, key);
    if (attr_val != nullptr) {
        std::string out;
        ::ase::viewer::render::append_escaped(out, attr_val);
        return out;
    }
    return build_inline_content(node);
}

// ── Cairo helpers ──────────────────────────────────────────────────

inline void rounded_rect(const Cairo::RefPtr<Cairo::Context>& cr,
                         double x, double y, double w, double h, double r) {
    cr->move_to(x + r, y);
    cr->line_to(x + w - r, y);
    cr->arc(x + w - r, y + r, r, -M_PI / 2.0, 0.0);
    cr->line_to(x + w, y + h - r);
    cr->arc(x + w - r, y + h - r, r, 0.0, M_PI / 2.0);
    cr->line_to(x + r, y + h);
    cr->arc(x + r, y + h - r, r, M_PI / 2.0, M_PI);
    cr->line_to(x, y + r);
    cr->arc(x + r, y + r, r, M_PI, 3.0 * M_PI / 2.0);
    cr->close_path();
}

}  // namespace ase::viewer::render::dsl
