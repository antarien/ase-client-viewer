#pragma once

/**
 * @file        vwr_rndr_synt.hpp
 * @brief       syntax_highlight - colours a code block through tree-sitter
 * @description Declares the one entry point the code-block renderer calls. The
 *              body parses the code with tree-sitter and returns a Pango markup
 *              string whose spans carry the colours, for cpp, typescript, bash,
 *              json, yaml, python and cmake.
 *
 *              AN UNSUPPORTED LANGUAGE IS NOT AN ERROR: the function returns the
 *              plain text without markup, so the block renders unhighlighted
 *              instead of not rendering.
 *
 * @module      ase-client-viewer
 * @layer       5 (Servers & Clients)
 * @category    render/draw/command
 * @usecase     usecase:100008 - render the knowledge stock
 * @created     2026-04-12
 * @modified    2026-10-13
 * @version     00.00.35.00035 [core]
 */

#include <string>
#include <cstdint>

namespace ase::viewer::render {

// Generate Pango markup string with syntax-highlighted spans.
// Returns markup like: <span color="#569cd6">int</span> x = <span color="#b5cea8">0</span>;
// If language is unsupported, returns plain text (no markup).
std::string syntax_highlight(const char* code, uint32_t len, const char* language);

}  // namespace ase::viewer::render
