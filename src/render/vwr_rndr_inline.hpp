#pragma once

/**
 * @file        vwr_rndr_inline.hpp
 * @brief       Shared inline-text utilities - markup, escaping, plain text
 * @description Carries what every block renderer needs for its inline content:
 *              streq and cstr_len in place of std::strcmp and std::strlen,
 *              append_escaped for the &, <, >, " and ' of Pango markup, and the
 *              build_inline_* pair that walks the inline children of a block
 *              node into one Pango markup string - bold, italic, strike, inline
 *              code, links, NerdFont icons, glossary terms, wiki links, cross
 *              references and the version insert.
 *
 *              NerdFont icons are looked up in the build-time generated
 *              render/nerdfont_table.hpp and emitted with a Pango font_family
 *              span pointing at "Symbols Nerd Font Mono".
 *
 *              INLINE MATH EMITS ITS LATEX AS ITALIC TEXT and does not draw
 *              glyphs: drawing them through ase::adp::microtex needs Cairo glyph
 *              injection inside a Pango layout, which this path does not do.
 *              Display math goes through vwr_rndr_math instead, where there is
 *              no Pango layout in the way.
 *
 *              Both vwr_rndr_doc.cpp, the block dispatcher, and the DSL
 *              directive renderers under src/render/dsl/ use this one header.
 *
 * @module      ase-client-viewer
 * @layer       5 (Servers & Clients)
 * @category    render/draw/command
 * @usecase     usecase:100008 - render the knowledge stock
 * @created     2026-04-13
 * @modified    2026-10-13
 * @version     00.00.35.00035 [core]
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
