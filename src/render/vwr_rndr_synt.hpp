#pragma once

/**
 * Syntax highlighting for code blocks using tree-sitter.
 *
 * Parses code content, produces colored spans for Pango markup.
 * Supported languages: cpp, typescript, bash, json, yaml, python, cmake.
 */

#include <string>
#include <cstdint>

namespace ase::viewer::render {

// Generate Pango markup string with syntax-highlighted spans.
// Returns markup like: <span color="#569cd6">int</span> x = <span color="#b5cea8">0</span>;
// If language is unsupported, returns plain text (no markup).
std::string syntax_highlight(const char* code, uint32_t len, const char* language);

}  // namespace ase::viewer::render
