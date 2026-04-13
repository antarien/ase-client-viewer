#pragma once

/**
 * Shared inline-text utilities for the ASE Markdown viewer.
 *
 * Provides:
 *   • streq / cstr_len  — std::strcmp / std::strlen replacements
 *   • escape_pango      — escapes &, <, >, ", ' for Pango markup
 *   • build_inline_*    — walks inline children of a block node and
 *                         emits a Pango markup string (bold, italic,
 *                         strike, inline code, links, NerdFont icons,
 *                         glossary, wiki-link, cross-ref, version)
 *
 * NerdFont icons are looked up via the build-time generated
 * render/nerdfont_table.hpp and emitted with a Pango font_family
 * span pointing at "Symbols Nerd Font Mono".
 *
 * Inline math currently renders the raw LaTeX in italic chartreuse —
 * proper glyph rendering through ase::microtex requires Cairo glyph
 * injection inside Pango layouts which is a follow-up.
 *
 * Used by both vwr_rndr_doc.cpp (block dispatcher) and the DSL
 * directive renderers under src/render/dsl/.
 *
 * @module      ase-client-viewer
 * @layer       5 (Clients)
 */

#include <ase/markdown/ast.hpp>

#include <cstdint>
#include <string>

namespace ase::viewer::render {

uint32_t cstr_len(const char* s);
bool     streq(const char* a, const char* b);

void append_escaped(std::string& out, const char* s, uint32_t len);
void append_escaped(std::string& out, const char* s);

void build_inline_markup(std::string& out, const ase::markdown::Node* node);
void build_inline_children(std::string& out, const ase::markdown::Node* parent);

// Walk every child (block or inline) and concatenate plain UTF-8 text only.
// Used as a fallback for cases where a Pango markup string is overkill
// (e.g. attribute values, simple labels).
void collect_plain_text(const ase::markdown::Node* node, std::string& out);

}  // namespace ase::viewer::render
