/**
 * Syntax highlighting — stub implementation.
 *
 * tree-sitter integration will be added when the FetchContent
 * dependency is wired up during build testing. For now, returns
 * Pango-escaped plain text so code blocks render correctly.
 */

#include "vwr_rndr_synt.hpp"
#include <cstring>

namespace ase::viewer::render {

namespace {

// Escape XML special characters for Pango markup
std::string escape_markup(const char* text, uint32_t len) {
    std::string result;
    result.reserve(len * 1.2);
    for (uint32_t i = 0; i < len; i++) {
        switch (text[i]) {
            case '&':  result += "&amp;"; break;
            case '<':  result += "&lt;"; break;
            case '>':  result += "&gt;"; break;
            case '"':  result += "&quot;"; break;
            case '\'': result += "&apos;"; break;
            default:   result += text[i]; break;
        }
    }
    return result;
}

}  // anonymous namespace

std::string syntax_highlight(const char* code, uint32_t len, const char* language) {
    (void)language;
    // Stub: return escaped text without highlighting
    // tree-sitter integration follows when FetchContent is wired
    return escape_markup(code, len);
}

}  // namespace ase::viewer::render
