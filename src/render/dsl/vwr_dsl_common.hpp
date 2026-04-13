#pragma once

/**
 * Shared utilities for DSL directive renderers.
 *
 * Color SSOT  — sha-client-web/sha-web-styles/src/colors.ts
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

#include <cmath>
#include <cstdint>
#include <string>

namespace ase::viewer::render::dsl {

// ── Color SSOT (sha-web-styles) — DOCS_* palette ───────────────────

constexpr double CYAN_R   = 0x80 / 255.0, CYAN_G   = 0xde / 255.0, CYAN_B   = 0xff / 255.0; // DOCS_H1
constexpr double CHART_R  = 0xba / 255.0, CHART_G  = 0xd0 / 255.0, CHART_B  = 0x2d / 255.0; // DOCS_H2
constexpr double ORANGE_R = 0xe0 / 255.0, ORANGE_G = 0xa0 / 255.0, ORANGE_B = 0x60 / 255.0; // DOCS_H3
constexpr double PURPLE_R = 0xa0 / 255.0, PURPLE_G = 0x80 / 255.0, PURPLE_B = 0xc0 / 255.0; // DOCS_H4

constexpr double GREEN_R  = 0x30 / 255.0, GREEN_G  = 0xa8 / 255.0, GREEN_B  = 0x48 / 255.0; // DOCS_CALLOUT_TIP_BORDER
constexpr double AMBER_R  = 0xc8 / 255.0, AMBER_G  = 0xa0 / 255.0, AMBER_B  = 0x30 / 255.0; // DOCS_CALLOUT_WARN_BORDER
constexpr double NOTE_R   = 0x90 / 255.0, NOTE_G   = 0x50 / 255.0, NOTE_B   = 0xc0 / 255.0; // DOCS_CALLOUT_NOTE_BORDER
constexpr double INFO_R   = 0x3a / 255.0, INFO_G   = 0x90 / 255.0, INFO_B   = 0xb8 / 255.0; // DOCS_CALLOUT_INFO_BORDER
constexpr double RED_R    = 0xa8 / 255.0, RED_G    = 0x4a / 255.0, RED_B    = 0x4a / 255.0; // PANEL_RED

constexpr double TEXT_R   = 0x8a / 255.0, TEXT_G   = 0x9a / 255.0, TEXT_B   = 0x9a / 255.0; // TEXT_PRIMARY
constexpr double MUTED_R  = 0x70 / 255.0, MUTED_G  = 0x70 / 255.0, MUTED_B  = 0x70 / 255.0; // TEXT_PANEL_MUTED
constexpr double DOCSFG_R = 0xdd / 255.0, DOCSFG_G = 0xdd / 255.0, DOCSFG_B = 0xdd / 255.0; // TEXT_DOCS_FG

constexpr double SURFACE_R     = 0x0c / 255.0, SURFACE_G     = 0x12 / 255.0, SURFACE_B     = 0x14 / 255.0; // DOCS_DSL_BG
constexpr double SURFACE_BRT_R = 0x0f / 255.0, SURFACE_BRT_G = 0x11 / 255.0, SURFACE_BRT_B = 0x0f / 255.0; // DOCS_TABLE_ROW_ALT

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
