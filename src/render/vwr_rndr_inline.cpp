/*
 * ==============================================================================
 * ASE VIEWER — Inline rendering helpers
 * ==============================================================================
 *
 * @file        vwr_rndr_inline.cpp
 * @brief       Shared inline-text helpers used by the document dispatcher
 *              and every DSL directive renderer.
 * @description Walks inline AST children of a block-level node and emits
 *              a Pango markup string. Supports emphasis/strong/strike,
 *              inline code, links, NerdFont icons (font_family span), wiki
 *              links, glossary terms, cross-references, version inserts and
 *              inline math (italic chartreuse placeholder). NerdFont glyphs
 *              are looked up via render/nerdfont_table.hpp which is
 *              auto-generated from clients/ase-client-web/src/utils/icon-replacer.ts.
 *
 * @module      ase-client-viewer
 * @layer       5 (Clients)
 * @category    process/computation/algorithm
 *
 * @created     2026-04-13
 * @modified    2026-04-13
 * @version     00.00.01.00001 [poc]
 *
 * ==============================================================================
 * CLIENT INFRASTRUCTURE COMPLIANCE
 * ==============================================================================
 * [ ] No raw new/delete (smart pointers or stack only)
 * [ ] No std::vector / std::map / std::set in source-level data structures
 * [ ] No switch/case dispatch — use if/else ladders
 * [ ] No std::stringstream / std::strcmp / std::strlen
 * [ ] All inline rich-text routed through Pango markup
 * [ ] NerdFont icons rendered via build-generated nerdfont_table.hpp
 * ==============================================================================
 */

#include "vwr_rndr_inline.hpp"

#include "render/nerdfont_table.hpp"

#include <ase/markdown/types.hpp>

namespace ase::viewer::render {

uint32_t cstr_len(const char* s) {
    if (s == nullptr) return 0;
    uint32_t n = 0;
    while (s[n] != '\0') ++n;
    return n;
}

bool streq(const char* a, const char* b) {
    if (a == nullptr || b == nullptr) return false;
    uint32_t i = 0;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) return false;
        ++i;
    }
    return a[i] == b[i];
}

void append_escaped(std::string& out, const char* s, uint32_t len) {
    if (s == nullptr || len == 0) return;
    for (uint32_t i = 0; i < len; ++i) {
        char c = s[i];
        if      (c == '&')  out.append("&amp;");
        else if (c == '<')  out.append("&lt;");
        else if (c == '>')  out.append("&gt;");
        else if (c == '"')  out.append("&quot;");
        else if (c == '\'') out.append("&apos;");
        else                out.push_back(c);
    }
}

void append_escaped(std::string& out, const char* s) {
    append_escaped(out, s, cstr_len(s));
}

void build_inline_children(std::string& out, const ase::markdown::Node* parent) {
    if (parent == nullptr) return;
    for (const ase::markdown::Node* c = parent->first_child; c != nullptr; c = c->next_sibling) {
        build_inline_markup(out, c);
    }
}

void build_inline_markup(std::string& out, const ase::markdown::Node* node) {
    if (node == nullptr) return;
    using namespace ase::markdown;

    if (node->type == NODE_TEXT) {
        append_escaped(out, node->text, node->text_len);
        return;
    }
    if (node->type == NODE_SOFTBREAK) {
        out.push_back(' ');
        return;
    }
    if (node->type == NODE_LINEBREAK) {
        out.push_back('\n');
        return;
    }
    if (node->type == NODE_EMPHASIS) {
        out.append("<i>");
        build_inline_children(out, node);
        out.append("</i>");
        return;
    }
    if (node->type == NODE_STRONG) {
        out.append("<b>");
        build_inline_children(out, node);
        out.append("</b>");
        return;
    }
    if (node->type == NODE_STRIKETHROUGH) {
        out.append("<s>");
        build_inline_children(out, node);
        out.append("</s>");
        return;
    }
    if (node->type == NODE_INLINE_CODE) {
        // Mirrors the web <code> styling from
        // clients/ase-client-web/ase-web-docviewer/src/DocsViewer.tsx:
        //   bg-docs-code-bg (#0c0d14) + border (#393745)
        //   + text-docs-link (#5a9cb8) + font-mono
        // Pango-Markup supports background + foreground per span.
        // Leading/trailing non-breaking thin spaces mimic the px-1
        // horizontal padding so the inline box looks distinct from the
        // surrounding paragraph text.
        out.append("<span font_family=\"Fira Code\" background=\"#0c0d14\" foreground=\"#5a9cb8\">\xe2\x80\x89");
        append_escaped(out, node->text, node->text_len);
        out.append("\xe2\x80\x89</span>");
        return;
    }
    if (node->type == NODE_LINK) {
        out.append("<span foreground=\"#5a9cb8\" underline=\"single\">");
        build_inline_children(out, node);
        out.append("</span>");
        return;
    }
    if (node->type == NODE_NERDFONT_ICON) {
        const char* utf8 = lookup_nerdfont(node->text, node->text_len);
        if (utf8 != nullptr) {
            out.append("<span font_family=\"Symbols Nerd Font Mono\" foreground=\"#80deff\">");
            out.append(utf8);
            out.append("</span>");
        } else {
            out.append("<span foreground=\"#5a9cb8\">(");
            append_escaped(out, node->text, node->text_len);
            out.append(")</span>");
        }
        return;
    }
    if (node->type == NODE_MATH_INLINE) {
        out.append("<i><span foreground=\"#bad02d\">");
        append_escaped(out, node->text, node->text_len);
        out.append("</span></i>");
        return;
    }
    if (node->type == NODE_WIKI_LINK) {
        out.append("<span foreground=\"#5a9cb8\">[[");
        append_escaped(out, node->text, node->text_len);
        out.append("]]</span>");
        return;
    }
    if (node->type == NODE_GLOSSARY_TERM) {
        out.append("<span foreground=\"#a690fa\" underline=\"low\">");
        append_escaped(out, node->text, node->text_len);
        out.append("</span>");
        return;
    }
    if (node->type == NODE_CROSS_REF) {
        out.append("<span foreground=\"#5a9cb8\">→ ");
        if (node->url != nullptr) append_escaped(out, node->url);
        else build_inline_children(out, node);
        out.append("</span>");
        return;
    }
    if (node->type == NODE_VERSION_INSERT) {
        out.append("<span foreground=\"#5a9cb8\">{version}</span>");
        return;
    }

    // Container fallback — recurse for unknown inline-ish wrappers.
    build_inline_children(out, node);
}

void collect_plain_text(const ase::markdown::Node* node, std::string& out) {
    if (node == nullptr) return;
    if (node->text != nullptr && node->text_len > 0) {
        out.append(node->text, node->text_len);
    }
    for (const ase::markdown::Node* c = node->first_child; c != nullptr; c = c->next_sibling) {
        collect_plain_text(c, out);
    }
}

}  // namespace ase::viewer::render
