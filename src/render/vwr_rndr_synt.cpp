/**
 * Syntax highlighting via tree-sitter.
 *
 * Parses code with language-specific grammars, walks the syntax tree,
 * and produces Pango markup with colored spans per token type.
 *
 * Colors from SSOT: sha-web-styles/src/colors.ts (vscDarkPlus-inspired)
 */

#include "vwr_rndr_synt.hpp"
#include <tree_sitter/api.h>
#include <cstring>
#include <string>

// tree-sitter language declarations (from grammar libs)
extern "C" {
    const TSLanguage* tree_sitter_cpp();
    const TSLanguage* tree_sitter_c();
    const TSLanguage* tree_sitter_python();
    const TSLanguage* tree_sitter_bash();
    const TSLanguage* tree_sitter_json();
    const TSLanguage* tree_sitter_typescript();
}

namespace ase::viewer::render {

namespace {

// Token colors (vscDarkPlus-inspired, matching web syntax theme)
constexpr const char* COL_KEYWORD   = "#569cd6";  // blue — keywords
constexpr const char* COL_TYPE      = "#4ec9b0";  // teal — types, classes
constexpr const char* COL_STRING    = "#ce9178";  // orange — strings
constexpr const char* COL_NUMBER    = "#b5cea8";  // green — numbers
constexpr const char* COL_COMMENT   = "#6a9955";  // green — comments
constexpr const char* COL_FUNCTION  = "#dcdcaa";  // yellow — function names
constexpr const char* COL_OPERATOR  = "#d4d4d4";  // light gray — operators
constexpr const char* COL_PREPROC   = "#c586c0";  // purple — preprocessor
constexpr const char* COL_CONSTANT  = "#4fc1ff";  // bright blue — constants
constexpr const char* COL_PARAM     = "#9cdcfe";  // light blue — parameters

// Map tree-sitter node type to color
const char* node_type_color(const char* type) {
    if (!type) return nullptr;

    // Keywords
    if (std::strstr(type, "keyword") || std::strcmp(type, "if") == 0 ||
        std::strcmp(type, "else") == 0 || std::strcmp(type, "for") == 0 ||
        std::strcmp(type, "while") == 0 || std::strcmp(type, "return") == 0 ||
        std::strcmp(type, "class") == 0 || std::strcmp(type, "struct") == 0 ||
        std::strcmp(type, "enum") == 0 || std::strcmp(type, "namespace") == 0 ||
        std::strcmp(type, "template") == 0 || std::strcmp(type, "using") == 0 ||
        std::strcmp(type, "const") == 0 || std::strcmp(type, "static") == 0 ||
        std::strcmp(type, "virtual") == 0 || std::strcmp(type, "override") == 0 ||
        std::strcmp(type, "public") == 0 || std::strcmp(type, "private") == 0 ||
        std::strcmp(type, "protected") == 0 || std::strcmp(type, "auto") == 0 ||
        std::strcmp(type, "void") == 0 || std::strcmp(type, "def") == 0 ||
        std::strcmp(type, "import") == 0 || std::strcmp(type, "from") == 0 ||
        std::strcmp(type, "function") == 0 || std::strcmp(type, "let") == 0 ||
        std::strcmp(type, "var") == 0 || std::strcmp(type, "export") == 0 ||
        std::strcmp(type, "default") == 0 || std::strcmp(type, "case") == 0 ||
        std::strcmp(type, "switch") == 0 || std::strcmp(type, "break") == 0 ||
        std::strcmp(type, "continue") == 0 || std::strcmp(type, "try") == 0 ||
        std::strcmp(type, "catch") == 0 || std::strcmp(type, "throw") == 0 ||
        std::strcmp(type, "new") == 0 || std::strcmp(type, "delete") == 0 ||
        std::strcmp(type, "true") == 0 || std::strcmp(type, "false") == 0 ||
        std::strcmp(type, "null") == 0 || std::strcmp(type, "nullptr") == 0 ||
        std::strcmp(type, "constexpr") == 0 || std::strcmp(type, "inline") == 0)
        return COL_KEYWORD;

    // Types
    if (std::strstr(type, "type_identifier") || std::strstr(type, "primitive_type") ||
        std::strstr(type, "sized_type") || std::strcmp(type, "type") == 0)
        return COL_TYPE;

    // Strings
    if (std::strstr(type, "string") || std::strstr(type, "char_literal") ||
        std::strstr(type, "raw_string"))
        return COL_STRING;

    // Numbers
    if (std::strstr(type, "number") || std::strstr(type, "integer") ||
        std::strstr(type, "float"))
        return COL_NUMBER;

    // Comments
    if (std::strstr(type, "comment"))
        return COL_COMMENT;

    // Function names
    if (std::strstr(type, "function") || std::strstr(type, "method") ||
        std::strstr(type, "call_expression"))
        return COL_FUNCTION;

    // Preprocessor
    if (std::strstr(type, "preproc") || std::strstr(type, "include") ||
        std::strstr(type, "define") || std::strstr(type, "decorator"))
        return COL_PREPROC;

    return nullptr;
}

// Escape XML for Pango markup
void escape_append(std::string& out, const char* text, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        switch (text[i]) {
            case '&':  out += "&amp;"; break;
            case '<':  out += "&lt;"; break;
            case '>':  out += "&gt;"; break;
            case '"':  out += "&quot;"; break;
            case '\'': out += "&apos;"; break;
            default:   out += text[i]; break;
        }
    }
}

// Get tree-sitter language for a language name
const TSLanguage* get_language(const char* lang) {
    if (!lang) return nullptr;
    if (std::strcmp(lang, "cpp") == 0 || std::strcmp(lang, "c++") == 0 ||
        std::strcmp(lang, "cxx") == 0 || std::strcmp(lang, "hpp") == 0)
        return tree_sitter_cpp();
    if (std::strcmp(lang, "c") == 0)
        return tree_sitter_c();
    if (std::strcmp(lang, "python") == 0 || std::strcmp(lang, "py") == 0)
        return tree_sitter_python();
    if (std::strcmp(lang, "bash") == 0 || std::strcmp(lang, "sh") == 0 ||
        std::strcmp(lang, "shell") == 0 || std::strcmp(lang, "zsh") == 0)
        return tree_sitter_bash();
    if (std::strcmp(lang, "json") == 0 || std::strcmp(lang, "jsonc") == 0)
        return tree_sitter_json();
    if (std::strcmp(lang, "typescript") == 0 || std::strcmp(lang, "ts") == 0 ||
        std::strcmp(lang, "tsx") == 0 || std::strcmp(lang, "javascript") == 0 ||
        std::strcmp(lang, "js") == 0 || std::strcmp(lang, "jsx") == 0)
        return tree_sitter_typescript();
    return nullptr;
}

// Walk tree-sitter tree, emit colored spans
void walk_tree(TSNode node, const char* source, uint32_t source_len,
               std::string& result, uint32_t& last_pos) {
    uint32_t start = ts_node_start_byte(node);
    uint32_t end = ts_node_end_byte(node);
    if (end > source_len) end = source_len;

    uint32_t child_count = ts_node_child_count(node);

    if (child_count == 0) {
        // Leaf node — emit with color
        // First emit any text between last_pos and start (whitespace, etc.)
        if (start > last_pos) {
            escape_append(result, source + last_pos, start - last_pos);
        }

        const char* type = ts_node_type(node);
        const char* color = node_type_color(type);

        if (color && end > start) {
            result += "<span foreground=\"";
            result += color;
            result += "\">";
            escape_append(result, source + start, end - start);
            result += "</span>";
        } else if (end > start) {
            escape_append(result, source + start, end - start);
        }
        last_pos = end;
    } else {
        // Internal node — recurse into children
        for (uint32_t i = 0; i < child_count; i++) {
            TSNode child = ts_node_child(node, i);
            walk_tree(child, source, source_len, result, last_pos);
        }
    }
}

}  // anonymous namespace

std::string syntax_highlight(const char* code, uint32_t len, const char* language) {
    const TSLanguage* lang = get_language(language);
    if (!lang) {
        // Unsupported language — return escaped plain text
        std::string result;
        escape_append(result, code, len);
        return result;
    }

    TSParser* parser = ts_parser_new();
    ts_parser_set_language(parser, lang);

    TSTree* tree = ts_parser_parse_string(parser, nullptr, code, len);
    TSNode root = ts_tree_root_node(tree);

    std::string result;
    result.reserve(len * 2);
    uint32_t last_pos = 0;

    walk_tree(root, code, len, result, last_pos);

    // Emit any remaining text
    if (last_pos < len) {
        escape_append(result, code + last_pos, len - last_pos);
    }

    ts_tree_delete(tree);
    ts_parser_delete(parser);

    return result;
}

}  // namespace ase::viewer::render
